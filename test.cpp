
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <X11/keysym.h>
#include <X11/extensions/Xcomposite.h>
#include <math.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <chrono>

using namespace std;

static Display* dpy;



int singleBufferAttributes[] = {
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE,   GLX_RGBA_BIT,
	GLX_RED_SIZE,      1,   /* Request a single buffered color buffer */
	GLX_GREEN_SIZE,    1,   /* with the maximum number of color bits  */
	GLX_BLUE_SIZE,     1,   /* for each component                     */
	None
};

int doubleBufferAttributes[] = {
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE,   GLX_RGBA_BIT,
	GLX_DOUBLEBUFFER,  True,  /* Request a double-buffered color buffer with */
	GLX_RED_SIZE,      1,     /* the maximum number of bits per component    */
	GLX_GREEN_SIZE,    1, 
	GLX_BLUE_SIZE,     1,
	None
};

double map(double val, double min, double max, double newMax, double newMin) {
	return (val-min)/(max-min) * (newMax-newMin) + newMin;
}

int main( int argc, char *argv[] )
{
	Window                xWin;
	
	dpy = XOpenDisplay( NULL );
	if ( dpy == NULL ) {
		printf( "Unable to open a connection to the X server\n" );
		exit( EXIT_FAILURE );
	}
	
	XVisualInfo          *vInfo;
	GLXFBConfig          *fbConfigs;
	XSetWindowAttributes  swa;
	int                   swaMask;
	int                   numReturned;

	fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
		singleBufferAttributes, &numReturned );
	
	vInfo = glXGetVisualFromFBConfig( dpy, fbConfigs[0] );

	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	swa.colormap = XCreateColormap( dpy, RootWindow(dpy, vInfo->screen),
		vInfo->visual, AllocNone );
	swaMask = CWBorderPixel | CWColormap | CWEventMask;
	
	Window root = DefaultRootWindow(dpy);
	
	xWin = XCreateWindow( dpy, root, 1, 1, 2560, 1440,
		0, vInfo->depth, InputOutput, vInfo->visual,
		swaMask, &swa );
	XMapWindow( dpy, xWin );
	XFlush(dpy);
	
	GLXContext            context;
	GLXWindow             glxWin;
	
	context = glXCreateNewContext(dpy, fbConfigs[0], GLX_RGBA_TYPE,
	NULL, True );
	
	glxWin = glXCreateWindow( dpy, fbConfigs[0], xWin, NULL );
	
	glXMakeContextCurrent( dpy, glxWin, glxWin, context );
	
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_TEXTURE_2D);
    //glMatrixMode( GL_PROJECTION );
    
    while(1) {
    	//glViewport(0, 0, 2560, 1440);
		
		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		//glLoadIdentity();
		for(int i = 0; i < 1000000; i++) {
			glBegin(GL_POLYGON);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(10.0f, 0.0f);
			glVertex2f(10.0f, 10.0f);
			glVertex2f(0.0f, 10.0f);
			glEnd();
		}
		//glFlush();
	}
	
	exit( EXIT_SUCCESS );
}
