#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SplatTestApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void SplatTestApp::setup()
{
}

void SplatTestApp::mouseDown( MouseEvent event )
{
}

void SplatTestApp::update()
{
}

void SplatTestApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( SplatTestApp, RendererGl )
