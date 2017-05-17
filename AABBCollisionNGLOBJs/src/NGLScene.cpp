      #include <QMouseEvent>
  #include <QGuiApplication>
  #include <QFont>

  #include "NGLScene.h"
  #include <ngl/Camera.h>
  #include <ngl/Light.h>
  #include <ngl/Transformation.h>
  #include <ngl/Material.h>
  #include <ngl/NGLInit.h>
  #include <ngl/VAOPrimitives.h>
  #include <ngl/ShaderLib.h>

    #include <ngl/NGLStream.h>

    static float xmove=5;
    static float ymove=2;
    static float zmove=2;


  NGLScene::NGLScene(const std::string &_oname, const std::string &_tname)
  {
    setTitle("Obj Demo");
    m_showBBox=true;
    m_showBSphere=true;
    m_objFileName=_oname;
    m_textureFileName=_tname;
  }


  NGLScene::~NGLScene()
  {
    std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";

  }

  void NGLScene::resizeGL( int _w, int _h )
  {
    m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
    m_win.width  = static_cast<int>( _w * devicePixelRatio() );
    m_win.height = static_cast<int>( _h * devicePixelRatio() );
  }

  void NGLScene::initializeGL()
  {
    // we must call this first before any other GL commands to load and link the
    // gl commands from the lib, if this is not done program will crash
    ngl::NGLInit::instance();

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
    // enable depth testing for drawing
    glEnable(GL_DEPTH_TEST);
    // enable multisampling for smoother drawing
    glEnable(GL_MULTISAMPLE);

    // Now we will create a basic Camera from the graphics library
    // This is a static camera so it only needs to be set once
    // First create Values for the camera position
    ngl::Vec3 from(0,4,8);
    ngl::Vec3 to(0,0,0);
    ngl::Vec3 up(0,1,0);
    m_cam.set(from,to,up);
    // set the shape using FOV 45 Aspect Ratio based on Width and Height
    // The final two are near and far clipping planes of 0.5 and 10
    m_cam.setShape(45.0f,720.0f/576.0f,0.05f,350.0f);
    // now to load the shader and set the values
    // grab an instance of shader manager
    // grab an instance of shader manager
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    // load a frag and vert shaders

    shader->createShaderProgram("TextureShader");

    shader->attachShader("TextureVertex",ngl::ShaderType::VERTEX);
    shader->attachShader("TextureFragment",ngl::ShaderType::FRAGMENT);
    shader->loadShaderSource("TextureVertex","shaders/TextureVertex.glsl");
    shader->loadShaderSource("TextureFragment","shaders/TextureFragment.glsl");

    shader->compileShader("TextureVertex");
    shader->compileShader("TextureFragment");
    shader->attachShaderToProgram("TextureShader","TextureVertex");
    shader->attachShaderToProgram("TextureShader","TextureFragment");

    // link the shader no attributes are bound
    shader->linkProgramObject("TextureShader");
    (*shader)["TextureShader"]->use();



    (*shader)["nglColourShader"]->use();

    shader->setShaderParam4f("Colour",1.0,1.0,1.0,1.0);

    // first we create a mesh from an obj passing in the obj file and texture
    m_mesh.reset(  new ngl::Obj(m_objFileName,m_textureFileName));
    m_mesh2.reset(  new ngl::Obj(m_objFileName,m_textureFileName));
    // now we need to create this as a VAO so we can draw it
    m_mesh->createVAO();
    m_mesh->calcBoundingSphere();
    m_mesh2->createVAO();
    m_mesh2->calcBoundingSphere();

     meshUpdatedCenter = m_mesh->getBBox().center();
     mesh2UpdatedCenter = m_mesh2->getBBox().center();
     mesh2UpdatedCenter.set(xmove,ymove,zmove);



     meshUpdatedHalfWidth = m_mesh->getBBox().width()/2.0f;
     meshUpdatedHalfHeight = m_mesh->getBBox().height()/2.0f;
     meshUpdatedHalfDepth = m_mesh->getBBox().depth()/2.0f;
     mesh2UpdatedHalfWidth = m_mesh2->getBBox().width()/2.0f;
     mesh2UpdatedHalfHeight = m_mesh2->getBBox().height()/2.0f;
     mesh2UpdatedHalfDepth = m_mesh2->getBBox().depth()/2.0f;


    ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
    prim->createSphere("sphere",1.0,20);
    // as re-size is not explicitly called we need to do this.
    glViewport(0,0,width(),height());
    m_text.reset(new ngl::Text(QFont("Arial",16)));
    m_text->setScreenSize(width(),height());
    m_text->setColour(1,1,1);

  }


  void NGLScene::loadMatricesToShader()
  {
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();

    ngl::Mat4 MVP=m_transform.getMatrix()*m_mouseGlobalTX*m_cam.getVPMatrix();

    shader->setShaderParamFromMat4("MVP",MVP);
  }

  //AABB collision detection
  ///in detail (check each individual axis for..when the difference/distance
  ///between the centers of the 2 BBoxes is less than the the difference/distance
  ///between the sum of their added halfwidths for that axes)
  ///
  /// ---HW1---             ---HW2---
  /// /////////             /////////
  /// /   c1  /             /   c2  /
  /// /   /---/-------------/---/   /     !!!!D1:(abs(c2-c1)) tested against HW1+HW2!!!!
  /// /       /      D1     /       /
  /// /////////             /////////
  //https://studiofreya.com/3d-math-and-physics/simple-aabb-vs-aabb-collision-detection/
  bool NGLScene::checkCollision(float meshUpdatedHalfWidth,
                                float meshUpdatedHalfHeight,
                                float meshUpdatedHalfDepth,
                                float mesh2UpdatedHalfWidth,
                                float mesh2UpdatedHalfHeight,
                                float mesh2UpdatedHalfDepth)
  {


    bool x,y,z;

    //SIMD fast
    x = ( abs(meshUpdatedCenter.m_x - mesh2UpdatedCenter.m_x) <= (meshUpdatedHalfWidth + mesh2UpdatedHalfWidth) );
    std::cout<<meshUpdatedCenter.m_x<<" - "<<mesh2UpdatedCenter.m_x<<" > "<<meshUpdatedHalfWidth<<" + "<<mesh2UpdatedHalfHeight<<std::endl;

    y = ( abs(meshUpdatedCenter.m_y - mesh2UpdatedCenter.m_y) <= (meshUpdatedHalfHeight + mesh2UpdatedHalfHeight) );

    z = ( abs(meshUpdatedCenter.m_z - mesh2UpdatedCenter.m_z) <= (meshUpdatedHalfDepth + mesh2UpdatedHalfDepth) );

    // We have an overlap only when all three axes overlap
    return x && y && z;

  }


  void NGLScene::paintGL()
  {
    // clear the screen and depth buffer
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glViewport(0,0,m_win.width,m_win.height);
     ngl::Mat4 rotX;
     ngl::Mat4 rotY;
     // create the rotation matrices
     rotX.rotateX(m_win.spinXFace);
     rotY.rotateY(m_win.spinYFace);
     // multiply the rotations
     m_mouseGlobalTX=rotY*rotX;
     // add the translations
     m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
     m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
     m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;


    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["TextureShader"]->use();
    m_transform.reset();
    loadMatricesToShader();
    // draw the mesh
    m_mesh->draw();
    m_transform.reset();


    //std::cout<<"mesh2UpdatedMinX="<<mesh2UpdatedMinX<<std::endl;//used std::endl cause I wanted to see the ouput NOT in the terminal

    ngl::BBox b;

    //save before updating
    prevMesh2Center.set(mesh2UpdatedCenter);

    //update mesh2 game position if they don't overlap
    mesh2UpdatedCenter.set(xmove,ymove,zmove);

    bool overlap=checkCollision( meshUpdatedHalfWidth,
                                 meshUpdatedHalfHeight,
                                 meshUpdatedHalfDepth,
                                 mesh2UpdatedHalfWidth,
                                 mesh2UpdatedHalfHeight,
                                 mesh2UpdatedHalfDepth);


    std::cout<<"overlap="<<overlap<<std::endl;

    if (!overlap)//not overlapping
    {


        m_transform.setPosition(mesh2UpdatedCenter);

        // draw the mesh
        loadMatricesToShader();
        m_mesh2->draw();

        // draw the mesh bounding box
        (*shader)["nglColourShader"]->use();

        if(m_showBBox==true)
        {
          m_transform.reset();
          loadMatricesToShader();
          shader->setShaderParam4f("Colour",0,0,1,1);
          m_mesh->drawBBox();


          m_transform.setPosition(mesh2UpdatedCenter);

          loadMatricesToShader();
          std::cout<<"m_mesh2->getCenter()"<<m_mesh2->getCenter()<<"\n";
          m_mesh2->drawBBox();
        }

        if(m_showBSphere==true)
        {
          ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
          shader->setShaderParam4f("Colour",1,1,1,1);
          m_transform.reset();
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    //          m_transform.setPosition(m_mesh->getSphereCenter());
    //          m_transform.setScale(m_mesh->getSphereRadius(),m_mesh->getSphereRadius(),m_mesh->getSphereRadius());
    //          loadMatricesToShader();
    //          prim->draw("sphere");
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        }

    }
    else//overlapping AABBs
    {
        //go back to the prevMesh2Center, as we don't want to move it if they overlap
        mesh2UpdatedCenter.set(prevMesh2Center);
        xmove=mesh2UpdatedCenter.m_x;
        ymove=mesh2UpdatedCenter.m_y;
        zmove=mesh2UpdatedCenter.m_z;

        m_transform.setPosition(mesh2UpdatedCenter);

        // draw the mesh
        loadMatricesToShader();
        m_mesh2->draw();

        // draw the mesh bounding box
        (*shader)["nglColourShader"]->use();

        if(m_showBBox==true)
        {
          m_transform.reset();
          loadMatricesToShader();
          shader->setShaderParam4f("Colour",0,0,1,1);
          m_mesh->drawBBox();


          m_transform.setPosition(mesh2UpdatedCenter);

          loadMatricesToShader();
          std::cout<<"m_mesh2->getCenter()"<<m_mesh2->getCenter()<<"\n";
          m_mesh2->drawBBox();
        }

        if(m_showBSphere==true)
        {
          ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
          shader->setShaderParam4f("Colour",1,1,1,1);
          m_transform.reset();
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    //          m_transform.setPosition(m_mesh->getSphereCenter());
    //          m_transform.setScale(m_mesh->getSphereRadius(),m_mesh->getSphereRadius(),m_mesh->getSphereRadius());
    //          loadMatricesToShader();
    //          prim->draw("sphere");
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        }

    }


    m_text->renderText(10,18,"P toggle bounding Sphere B Toggle Bounding Box");

  }

  //----------------------------------------------------------------------------------------------------------------------

  void NGLScene::keyPressEvent(QKeyEvent *_event)
  {
    // this method is called every time the main window recives a key event.
    // we then switch on the key value and set the camera in the GLWindow
    switch (_event->key())
    {
    // escape key to quite
    case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
    // turn on wirframe rendering
    case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
    // turn off wire frame
    case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
    // show full screen
    case Qt::Key_F : showFullScreen(); break;
    // show windowed
    case Qt::Key_N : showNormal(); break;
    case Qt::Key_B : m_showBBox^=true; break;
    case Qt::Key_P : m_showBSphere^=true; break;
    case Qt::Key_Space :
          m_win.spinXFace=0;
          m_win.spinYFace=0;
          m_modelPos.set(ngl::Vec3::zero());
    break;

        case Qt::Key_Left : xmove-=0.1;  break;
        case Qt::Key_Right : xmove+=0.1; break;
        case Qt::Key_Up : ymove-=0.1; break; break;
        case Qt::Key_Down : ymove+=0.1; break;  break;
        case Qt::Key_E : zmove-=0.1; break; break;
        case Qt::Key_D : zmove+=0.1; break; break;


    default : break;
    }
    // finally update the GLWindow and re-draw
    update();
  }
