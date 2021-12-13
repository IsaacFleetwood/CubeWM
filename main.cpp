
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

long double curtime() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
}


using namespace std;

ofstream myfile;

class Texture;
class Vector;
class Block;
class Chunk;
class World;

class Vector {
	public:
		double x, y, z;
		
		Vector(double x, double y, double z) : x(x), y(y), z(z) { }
		
		int intX() { return (int) x; }
		int intY() { return (int) y; }
		int intZ() { return (int) z; }
		
		Vector* clone() { return new Vector(x, y, z); }
};

class Texture {
	public:
		/*
		int textureId;
		
		GLXPixmap* glxPixmap;
		
		Texture() {}
		
		Texture(GLXPixmap* glxPixmap, int textureId) : 
			glxPixmap(glxPixmap),
			textureId(textureId) {			
		}
		*/
		int type;
		GLuint textInt;
		char* name;
		Pixmap* pixmap;
		
		Texture() {
			name = "dirt.png";
			type = 0;
		}
		
		Texture(char* name) : name(name) {
			type = 0;
		}
		
		Texture(Pixmap* pixmap) : pixmap(pixmap) {
			type = 1;
		}

};
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);

static GLXFBConfig          *fbConfigs;
static Display* dpy;

static GLuint wrapper_glx_set_texture(Texture* texture, Texture* alt) {
	if(texture->type == 1 && !(texture->pixmap)) {
		texture = alt;
	}
	if(texture->textInt)
		return texture->textInt;
	if(texture->type == 0) {
		SDL_Surface *surface;
		GLuint textureInt;
		surface = IMG_Load(texture->name);
		glGenTextures(1, &textureInt);
		glBindTexture(GL_TEXTURE_2D, textureInt);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		SDL_FreeSurface(surface);
		texture->textInt = textureInt;
		return textureInt;
	} else {
		GLXFBConfig * configs = 0;

		const int pixmap_config[] = {
			GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
			GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
			GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
			GLX_DOUBLEBUFFER, False,
			//GLX_Y_INVERTED_EXT, GLX_DONT_CARE,
			None
		};

		const int pixmap_attribs[] = {
			GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
			GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
			None
		};
		GLuint texture_id;
		
    	int c = 0;
	
		configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), pixmap_config, &c);
		
		GLXPixmap glxpixmap = glXCreatePixmap(dpy, fbConfigs[0], *texture->pixmap, pixmap_attribs);
		
		t_glx_bind glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
		
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glXBindTexImageEXT(dpy, glxpixmap, GLX_FRONT_EXT, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
		
		texture->textInt = texture_id;
		return texture_id;
	}
	
}

class World {
	public:
		
		Chunk* chunks[2][1][2];
		
		World();
		
		bool hasBlock(Vector* v);
		bool setBlock(Vector* v, Block* b);
		Block* getBlock(Vector* v);
		
		void render();
		
};

Texture* dirt;

/* 
* Class used for giving blocks a texture. 
* 
* With the abstraction of blocks in the program, 
* all a block is is how it looks, so nothing else identifies it.
*
*/
class Block {
	public:
	
		Texture* textures[6];
		
		Block(Texture* tex)
		{
			//this->textures = new Texture[6];
			for(int i = 0; i < 6; i++) {
				this->textures[i] = tex;
			}
		}
		
		Block(Texture* side, Texture* up, Texture* down)
		{
			//this->textures = new Texture[6];
			this->textures[0] = side;
			this->textures[1] = side;
			this->textures[2] = up;
			this->textures[3] = down;
			this->textures[4] = side;
			this->textures[5] = side;
		}
		
		Block(
			Texture* tex_x_p, 
			Texture* tex_x_m, 
			Texture* tex_y_p, 
			Texture* tex_y_m, 
			Texture* tex_z_p, 
			Texture* tex_z_m
		)
		{
			//this->textures = new Texture*[6];
			this->textures[0] = tex_x_p;
			this->textures[1] = tex_x_m;
			this->textures[2] = tex_y_p;
			this->textures[3] = tex_y_m;
			this->textures[4] = tex_z_p;
			this->textures[5] = tex_z_m;
		}
		
		Block() { }
		
		void render(World* w, Vector* pos) {
			
			Vector* tempVec = new Vector(pos->x, pos->y, pos->z);
			
			for(int dx = 0; dx < 2; dx++) {
				tempVec->x = pos->x + (dx << 1)-1;
				if(!(w->hasBlock(tempVec))) {
					GLuint index = wrapper_glx_set_texture(this->textures[dx], dirt);
					this->renderRectangleX(dx, pos, index);
				}
			}
			tempVec->x = pos->x;
			for(int dy = 0; dy < 2; dy++) {
				tempVec->y = pos->y + (dy << 1)-1;
				if(!(w->hasBlock(tempVec))) {
					GLuint index = wrapper_glx_set_texture(this->textures[2|dy], dirt);
					this->renderRectangleY(dy, pos, index);
				}
			}
			tempVec->y = pos->y;
			
			
			for(int dz = 0; dz < 2; dz++) {
				tempVec->z = pos->z + (dz << 1)-1;
				if(!(w->hasBlock(tempVec))) {
					GLuint index = wrapper_glx_set_texture(this->textures[4|dz], dirt);
					this->renderRectangleZ(dz, pos, index);
				}
			}
			delete tempVec;
			
		}
		
		void renderRectangleX(int dx, Vector* pos, GLuint textureIndex) {
			double newX = pos->x + dx;
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glBegin(GL_POLYGON);
			//glColor3f(  dx,  1.0,  1.0 );
			glTexCoord2f(dx, 1.0f);
			glVertex3f(newX, pos->y, pos->z);
			glTexCoord2f(dx, 0.0f);
			glVertex3f(newX, pos->y+1, pos->z);
			glTexCoord2f(1.0f-dx, 0.0f);
			glVertex3f(newX, pos->y+1, pos->z+1);
			glTexCoord2f(1.0f-dx, 1.0f);
			glVertex3f(newX, pos->y, pos->z+1);
			glEnd();
		}
		
		void renderRectangleY(int dy, Vector* pos, GLuint textureIndex) {
			double newY = pos->y + dy;
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glBegin(GL_POLYGON);
			//glColor3f(  1.0,  dy,  1.0 );
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(pos->x, newY, pos->z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(pos->x+1, newY, pos->z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(pos->x+1, newY, pos->z+1);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(pos->x, newY, pos->z+1);
			glEnd();
		}
		
		void renderRectangleZ(int dz, Vector* pos, GLuint textureIndex) {
			double newZ = pos->z + dz;
			
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glBegin(GL_POLYGON);
			//glColor3f(  1.0,  1.0,  dz );
			
			glTexCoord2f(1.0f-dz, 1.0f);
			glVertex3f(pos->x, pos->y, newZ);
			glTexCoord2f(1.0f-dz, 0.0f);
			glVertex3f(pos->x, pos->y+1, newZ);
			glTexCoord2f(dz, 0.0f);
			glVertex3f(pos->x+1, pos->y+1, newZ);
			glTexCoord2f(dz, 1.0f);
			glVertex3f(pos->x+1, pos->y, newZ);
			
			glEnd();
		}
		
};

Block* BLOCK_GRASS = new Block(new Texture("grass.png"));
Block* BLOCK_WOOD = new Block(new Texture("planks_oak.png"));
Block* BLOCK_COBBLESTONE = new Block(new Texture("cobblestone.png"));
Block* BLOCK_DIRT = new Block(new Texture("dirt.png"));
Block* BLOCK_AIR = new Block();
Texture* winTexture = new Texture("dirt.png");


class Chunk {
	public:
	
		int i, j, k;
		Block* blocks[16][16][16];
	
		Chunk() {}
	
		Chunk(int i, int j, int k) : i(i), j(j), k(k) {
			for(int x = 0; x < 16; x++) {
				for(int y = 0; y < 16; y++) {
					for(int z = 0; z < 16; z++) {
						this->blocks[x][y][z] = NULL;
					}
				}
			}
		}
		
		void fillWithGrass() {
			for(int x = 0; x < 16; x++) {
				for(int z = 0; z < 16; z++) {
					this->blocks[x][0][z] = BLOCK_GRASS;
				}
			}
			for(int x = 4; x < 12; x++) {
				for(int z = 4; z < 12; z++) {
					if(x == 4 || z == 4 || x == 11 || z == 11) {
						for(int y = 0; y < 10; y++) {
							this->blocks[x][y][z] = BLOCK_COBBLESTONE;
						}
					}
					this->blocks[x][0][z] = BLOCK_WOOD;
				}
			}
			this->blocks[4][1][9] = NULL;
			this->blocks[4][2][9] = NULL;
		}
		
		void render(World* w) {
			int chunkX = i << 4;
			int chunkY = j << 4;
			int chunkZ = k << 4;
			Vector* v = new Vector(0, 0, 0);
			for(int x = 0; x < 16; x++) {
				v->x = chunkX | x;
				for(int y = 0; y < 16; y++) {
					v->y = chunkY | y;
					for(int z = 0; z < 16; z++) {
						v->z = chunkZ | z;
						
						if(!this->blocks[x][y][z])
							continue;
						this->blocks[x][y][z]->render(w, v);
					}
				}
			}
			delete v;
		}
		
};

World::World() {
	for(int i = 0; i < 2; i++) {
		for(int j = 0; j < 1; j++) {
			for(int k = 0; k < 2; k++) {
				this->chunks[i][j][k] = new Chunk(i, j, k);
			}
		}
	}
	for(int i = 0; i < 2; i++) {
		for(int j = 0; j < 2; j++) {
			this->chunks[i][0][j]->fillWithGrass();
		}
	}
}

// World declare function
bool World::hasBlock(Vector* v) {
	if(v->intX() < 0 || v->intY() < 0 || v->intZ() < 0)
		return false;
	if((v->intX()>>4) >= 2 || (v->intY()>>4) >= 1 || (v->intZ()>>4) >= 2)
		return false;
	Chunk* chunk;
	if(!(chunk = this->chunks[v->intX()>>4][v->intY()>>4][v->intZ()>>4]))
		return false;
	
	if(chunk->blocks[v->intX() & 0xF][v->intY() & 0xF][v->intZ() & 0xF] == NULL) {
		return false;
	}
	return true;
}

bool World::setBlock(Vector* v, Block* block) {
	if(v->intX() < 0 || v->intY() < 0 || v->intZ() < 0)
		return false;
	if((v->intX()>>4) >= 2 || (v->intY()>>4) >= 1 || (v->intZ()>>4) >= 2)
		return false;
	Chunk* chunk;
	if(!(chunk = this->chunks[v->intX()>>4][v->intY()>>4][v->intZ()>>4]))
		return false;
	
	chunk->blocks[v->intX() & 0xF][v->intY() & 0xF][v->intZ() & 0xF] = block;
	return true;
}

Block* World::getBlock(Vector* v) {
	if(v->intX() < 0 || v->intY() < 0 || v->intZ() < 0)
		return NULL;
	if((v->intX()>>4) >= 2 || (v->intY()>>4) >= 1 || (v->intZ()>>4) >= 2)
		return NULL;
	Chunk* chunk;
	if(!(chunk = this->chunks[v->intX()>>4][v->intY()>>4][v->intZ()>>4]))
		return NULL;
	
	return chunk->blocks[v->intX() & 0xF][v->intY() & 0xF][v->intZ() & 0xF];
}

void World::render() {
	for(int i = 0; i < 2; i++) {
		for(int j = 0; j < 2; j++) {
			this->chunks[i][0][j]->render(this);
		}
	}
}

int singleBufferAttributess[] = {
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
	GLX_DEPTH_SIZE,    1,
	None
};


static Bool WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
	return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}

Texture* cursor_texture = new Texture("left_ptr");

void draw_cursor(double x, double y, double z, double wid, double hei) {
	// cursor_texture
	GLuint texId = wrapper_glx_set_texture(winTexture, dirt);
	
	glBindTexture(GL_TEXTURE_2D, texId);
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(x, y-hei*0.1, z);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(x+wid*0.1, y-hei*0.1, z);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(x+wid*0.1, y, z);
	glEnd();
	
}

double map(double val, double min, double max, double newMax, double newMin) {
	return (val-min)/(max-min) * (newMax-newMin) + newMin;
}

int main( int argc, char *argv[] )
{
	
	int DISPLAY_WIDTH = 1920;
	int DISPLAY_HEIGHT = 1080;
	
	myfile.open("test.txt", std::ios_base::app);
	Window                xWin;
	dirt = new Texture("dirt.png");
	dpy = XOpenDisplay( NULL );
	if ( dpy == NULL ) {
		printf( "Unable to open a connection to the X server\n" );
		exit( EXIT_FAILURE );
	}
	
	/*
	int* stuff = new int;
	int* otherstuff = new int;
	std::cout << XCompositeQueryExtension(dpy, stuff, otherstuff) << "\n";
	*/
	
	XEvent                event;
	XVisualInfo          *vInfo;
	XSetWindowAttributes  swa;
	int                   swaMask;
	int                   numReturned;


	/* Open a connection to the X server */
	

	fbConfigs = glXChooseFBConfig( dpy, DefaultScreen(dpy),
		doubleBufferAttributes, &numReturned );
	
	/* Create an X colormap and window with a visual matching the first
	** returned framebuffer config */
	vInfo = glXGetVisualFromFBConfig( dpy, fbConfigs[0] );

	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	swa.colormap = XCreateColormap( dpy, RootWindow(dpy, vInfo->screen),
		vInfo->visual, AllocNone );
	swaMask = CWBorderPixel | CWColormap | CWEventMask;
	/*
	Window win = DefaultRootWindow(dpy);
	XWindowAttributes rootWinAttrs;
	XGetWindowAttributes(dpy, win, &rootWinAttrs);
	// rootWinAttrs.width, rootWinAttrs.height
	*/
	
	Window root = DefaultRootWindow(dpy);
	Window compositeWin = XCompositeGetOverlayWindow(dpy, root);;
	
	xWin = XCreateWindow( dpy, compositeWin, 1, 1, DISPLAY_WIDTH, DISPLAY_HEIGHT,
		0, vInfo->depth, InputOutput, vInfo->visual,
		swaMask, &swa );
	//*xWinPtr = xWin;
	XMapWindow( dpy, xWin );
	XFlush(dpy);

	//Window xWin = *xWinPtr;
	
	GLXContext            context;
	GLXWindow             glxWin;
	
	
	
	context = glXCreateNewContext(dpy, fbConfigs[0], GLX_RGBA_TYPE,
	NULL, True );
	
	
	glxWin = glXCreateWindow( dpy, fbConfigs[0], xWin, NULL );
	
	/*
	thread graphicsThread(updateGraphics, dpy, &xWin);
	
	while(!graphics_initialization_finished) {}
	*/
	
	// CirculateNotify | ConfigureNotify | CreateNotify | DestroyNotify | GravityNotify | MapNotify | MappingNotify | ReparentNotify | UnmapNotify | VisibilityNotify | 
	
	int eventMask = \
		ButtonPressMask | \
		ButtonReleaseMask | \
		ColormapChangeMask | \
		EnterWindowMask | \
		LeaveWindowMask | \
		ExposureMask | \
		FocusChangeMask | \
		KeymapStateMask | \
		KeyPressMask | \
		KeyReleaseMask | \
		PointerMotionMask | \
		PropertyChangeMask | \
		ResizeRedirectMask | \
		VisibilityChangeMask | \
		SubstructureNotifyMask | \
		SubstructureRedirectMask;
	
  	XSelectInput (dpy, root, eventMask);
	
	/* Map the window to the screen, and wait for it to appear */
	//while(true) {}
	/*
	Window win2 = XCreateWindow( dpy, root, 1, 1, DISPLAY_WIDTH, DISPLAY_HEIGHT,
		0, vInfo->depth, InputOutput, vInfo->visual,
		swaMask, &swa );
	XMapWindow( dpy, win2 );
	XFlush(dpy);
	GLXContext context2 = glXCreateNewContext(dpy, fbConfigs[0], GLX_RGBA_TYPE,
	NULL, True );
	GLXWindow glxWin2 = glXCreateWindow( dpy, fbConfigs[0], win2, NULL );
	
	glXMakeContextCurrent( dpy, glxWin2, glxWin2, context2 );
	glLineWidth(10);
	glBegin(GL_LINES);
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	glVertex2f(0,0);
	glVertex2f(100,100);
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glFlush();
	glXSwapBuffers( dpy, glxWin2 );
	*/
	
	glXMakeContextCurrent( dpy, glxWin, glxWin, context );
	
	//XCompositeRedirectWindow(dpy, win2, CompositeRedirectManual);
	/*
	Pixmap pixmap = XCompositeNameWindowPixmap(dpy, win2);
	Pixmap* ptr = &pixmap;
	*/
	
	int pixmapAmt = 1;
	int textureAmt = 1;
	int windowAmt = 1;
	Pixmap pixmaps[100];
	Texture textures[100];
	Window windows[100];
	
	Window selectedWindow;
	Window placeableWindow;
	
	XEvent ev;
	XEvent lastEv;
	
	double rotate_y = 0;
	double rotate_x = 0;
	
	int key[6] = {0,0,0,0,0,0};
	Vector* pos = new Vector(8.0,1.0,8.0);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
    glMatrixMode( GL_PROJECTION );
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    const double PI = 3.141592653589;
    double fov = PI/2;
    double scl = 0.0001f;
    double speed = 0.01;
    double minX = -sin(fov/2) * scl;
    double maxX = -minX;
    double minZ = cos(fov/2) * scl;
    double maxZ = 1024.0f;
    double minY = minX * 9.0/16.0;
    double maxY = -minY;
    double vY = 0;
    World* w = new World();
    long double time1, time2, firstTime;
    time2 = curtime();
    firstTime = time2;
    
    GLfloat linePoints[3];
    GLfloat linePoints2[3];
    
    int mouseX = 100;
    int mouseY = 100;
    bool mouseLock = true;
	bool ignoreNextMotion = false;
    
    while(1) {
    	glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    	//XNextEvent(dpy, &ev);
      	while(XCheckMaskEvent (dpy, eventMask, &ev)) {
      		if(ev.type != MotionNotify)
				myfile << ev.type << " \n";
			switch(ev.type) {
				case ColormapNotify:
					myfile << "Colormap Notify\n";
					break;
				case EnterNotify:
					myfile << "Enter Notify\n";
					break;
				case LeaveNotify:
					myfile << "Leave Notify\n";
					break;
				case Expose:
					myfile << "Expose Notify\n";
					break;
				case GraphicsExpose:
					myfile << "Graphics Expose\n";
					break;
				case NoExpose:
					myfile << "No Expose\n";
					break;
				case KeymapNotify:
					myfile << "Keymap \n";
					break;
				case PropertyNotify:
					myfile << "Property Notify\n";
					break;
				case ResizeRequest:
					myfile << "Resize QRequest\n";
					break;
				case VisibilityNotify:
					myfile << "Visibility Notify\n";
					break;
				case FocusIn:
					myfile << ev.type << " " << ev.xfocus.window << " " << xWin << " " << root << " Focus In \n";
					
					if(ev.xfocus.window != root) {
						XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
					}
					break;
				case FocusOut:
					myfile << ev.type << " " << ev.xfocus.window << " " << xWin << " " << root << " Focus Out \n";
					if(ev.xfocus.window == root) {
						XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
					}
					break;
				case CirculateNotify:
					myfile << ev.type << " Circulate Notify \n";
					break;
				case ConfigureNotify:
					myfile << ev.type << " " << ev.xconfigure.window << " Configure Notify \n";
					break;
				case GravityNotify:
					myfile << ev.type << " Gravity Notify \n";
					break;
				case MapNotify:
					myfile << ev.type << " " << ev.xmap.window << " Map Notify \n";
					break;
				case UnmapNotify:
					myfile << ev.type << " UnMap Notify \n";
					break;
				case CirculateRequest: {
					myfile << ev.type << " Circulate Request \n";
					/*
					XConfigureRequestEvent e = ev.xconfigurerequest;
					XWindowChanges changes;
					// Copy fields from e to changes.
					changes.x = e.x;
					changes.y = e.y;
					changes.width = e.width;
					changes.height = e.height;
					changes.border_width = e.border_width;
					changes.sibling = e.above;
					changes.stack_mode = e.detail;
					// Grant request by calling XConfigureWindow().
					XConfigureWindow(dpy, e.window, e.value_mask, &changes);
					*/
					break;
				}
				case ConfigureRequest: {
					myfile << ev.type << " " << ev.xconfigurerequest.window << " Configure Request \n";
					
					XConfigureRequestEvent e = ev.xconfigurerequest;
					XWindowChanges changes;
					// Copy fields from e to changes.
					changes.width = 512;//e.width;
					changes.height = 512;//e.height;
					changes.x = DISPLAY_WIDTH/2-changes.width/2;//e.x;
					changes.y = DISPLAY_HEIGHT/2-changes.height/2;//e.y;
					changes.border_width = e.border_width;
					changes.sibling = e.above;
					changes.stack_mode = e.detail;
					// Grant request by calling XConfigureWindow().
					XConfigureWindow(dpy, e.window, e.value_mask, &changes);
					
					break;
				}
				case CreateNotify:
					myfile << ev.type << " Create Notify \n";
					break;
				case DestroyNotify:
					myfile << ev.type << " Destroy Notify \n";
					if(selectedWindow == ev.xdestroywindow.window) {
						selectedWindow = NULL;
					}
					for(int i = 0; i < windowAmt; i++) {
						if(windows[i] == ev.xdestroywindow.window) {
							windows[i] = NULL;
							textures[i] = textures[0];
						}
					}
					break;
				case ReparentNotify:
					myfile << ev.type << " Reparent Notify \n";
					break;
				case MapRequest: {
					myfile << ev.type << " " << ev.xmaprequest.window << " Map Request \n";
					
					XMapWindow(dpy, ev.xmaprequest.window);
					
					XCompositeRedirectWindow(dpy, ev.xmaprequest.window, CompositeRedirectManual);
					windows[windowAmt] = ev.xmaprequest.window;
					pixmaps[pixmapAmt] = XCompositeNameWindowPixmap(dpy, ev.xmaprequest.window);
					placeableWindow = ev.xmaprequest.window;
					Texture winTexture = Texture(&pixmaps[pixmapAmt]);
					winTexture.type = 1;
					winTexture.pixmap = &pixmaps[pixmapAmt];
					winTexture.textInt = 0;
					textures[textureAmt] = winTexture;
					textureAmt += 1;
					windowAmt += 1;
					pixmapAmt += 1;
					XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
					break;
				}
				case KeyPress:
					myfile << "Key press " << ev.xkey.keycode << "\n";
					if(!mouseLock) {
						//myfile << "State " << ev.xkey.state << " " << Mod1Mask << "\n";
						if(ev.xkey.keycode == 52) {
							mouseLock = true;
							XWindowAttributes winAttrs;
							XGetWindowAttributes(dpy, root, &winAttrs);
							if(winAttrs.width == 0)
								break;
							XWarpPointer(dpy, None, root, 0, 0, 0, 0, winAttrs.width/2, winAttrs.height/2);
							ignoreNextMotion = true;
						} else if(windowAmt) {
							ev.xkey.window = selectedWindow;
							XSendEvent(dpy, selectedWindow, true, KeyPressMask, &ev);
						}
						break;
					}
					switch(ev.xkey.keycode) {
						case 23:
							system("urxvt &");
							break;
						case 38:
							key[0] = 1;
							break;
						case 40:
							key[1] = 1;
							break;
						case 25:
							key[2] = 1;
							break;
						case 39:
							key[3] = 1;
							break;
						case 65:
							key[4] = 1;
							break;
						case 50:
							key[5] = 1;
							break;
						case 52: {
							double dist;
							Vector* dir = new Vector(0,0,1);
							double newY = dir->y * cos(-rotate_x) - sin(-rotate_x) * dir->z;
							double newZ = dir->y * sin(-rotate_x) + cos(-rotate_x) * dir->z;
							dir->y = newY;
							dir->z = newZ;
							double newX = dir->x * cos(rotate_y) - sin(rotate_y) * dir->z;
								   newZ = dir->x * sin(rotate_y) + cos(rotate_y) * dir->z;
							dir->x = newX;
							dir->z = newZ;
							Vector* rayPos = new Vector(pos->x, pos->y+1.6, pos->z);
							linePoints[0] = (GLfloat) rayPos->x;
							linePoints[1] = (GLfloat) rayPos->y;
							linePoints[2] = (GLfloat) rayPos->z;
							double step = 0;
							XWindowAttributes winAttrs;
							XGetWindowAttributes(dpy, xWin, &winAttrs);
							if(winAttrs.width == 0)
								break;
							GLfloat depth_comp;
							glReadPixels(
								winAttrs.width/2, winAttrs.height/2, 
								1, 1, 
								GL_DEPTH_COMPONENT,
								GL_FLOAT,
								&depth_comp);
							if(depth_comp == 1.0f)
								break;
							//float realDepth = (maxZ-minZ)*depth_comp + minZ;
							GLfloat clip_z = (depth_comp - 0.5f) * 2.0f;
							GLfloat realDepth = 2*maxZ*minZ/(clip_z*(maxZ-minZ)-(maxZ+minZ));
							
							rayPos->x = rayPos->x + dir->x * realDepth;
							rayPos->y = rayPos->y + dir->y * realDepth;
							rayPos->z = rayPos->z + dir->z * realDepth;
							linePoints2[0] = (GLfloat) rayPos->x;
							linePoints2[1] = (GLfloat) rayPos->y;
							linePoints2[2] = (GLfloat) rayPos->z;
							
							int i = 0;
							
							while(!w->hasBlock(rayPos)) {
								rayPos->x = rayPos->x + dir->x * realDepth * 0.01;
								rayPos->y = rayPos->y + dir->y * realDepth * 0.01;
								rayPos->z = rayPos->z + dir->z * realDepth * 0.01;
								i += 1;
								if(i > 10) break;
							}
							Block* block = w->getBlock(rayPos);
							if(!block) {
								break;
							}
							Texture* text = block->textures[0];
							for(int i = 0; i < textureAmt; i++) {
								if(text == &textures[i]) {
									if(windows[i]) {
										selectedWindow = windows[i];
										mouseLock = false;
										break;
									}
								}
							}
							break;
						}
						case 53: {
							double dist;
							Vector* dir = new Vector(0,0,1);
							double newY = dir->y * cos(-rotate_x) - sin(-rotate_x) * dir->z;
							double newZ = dir->y * sin(-rotate_x) + cos(-rotate_x) * dir->z;
							dir->y = newY;
							dir->z = newZ;
							double newX = dir->x * cos(rotate_y) - sin(rotate_y) * dir->z;
								   newZ = dir->x * sin(rotate_y) + cos(rotate_y) * dir->z;
							dir->x = newX;
							dir->z = newZ;
							Vector* rayPos = new Vector(pos->x, pos->y+1.6, pos->z);
							linePoints[0] = (GLfloat) rayPos->x;
							linePoints[1] = (GLfloat) rayPos->y;
							linePoints[2] = (GLfloat) rayPos->z;
							double step = 0;
							XWindowAttributes winAttrs;
							XGetWindowAttributes(dpy, xWin, &winAttrs);
							if(winAttrs.width == 0)
								break;
							GLfloat depth_comp;
							glReadPixels(
								winAttrs.width/2, winAttrs.height/2, 
								1, 1, 
								GL_DEPTH_COMPONENT,
								GL_FLOAT,
								&depth_comp);
							if(depth_comp == 1.0f)
								break;
							//float realDepth = (maxZ-minZ)*depth_comp + minZ;
							GLfloat clip_z = (depth_comp - 0.5f) * 2.0f;
							GLfloat realDepth = 2*maxZ*minZ/(clip_z*(maxZ-minZ)-(maxZ+minZ));
							
							rayPos->x = rayPos->x + dir->x * realDepth;
							rayPos->y = rayPos->y + dir->y * realDepth;
							rayPos->z = rayPos->z + dir->z * realDepth;
							linePoints2[0] = (GLfloat) rayPos->x;
							linePoints2[1] = (GLfloat) rayPos->y;
							linePoints2[2] = (GLfloat) rayPos->z;
							
							int i = 0;
							
							while(!w->hasBlock(rayPos)) {
								rayPos->x = rayPos->x + dir->x * realDepth * 0.01;
								rayPos->y = rayPos->y + dir->y * realDepth * 0.01;
								rayPos->z = rayPos->z + dir->z * realDepth * 0.01;
								i += 1;
								if(i > 10) break;
							}
							Block* block = w->getBlock(rayPos);
							if(!block) {
								break;
							}
							Texture* text = block->textures[0];
							for(int i = 0; i < textureAmt; i++) {
								if(text == &textures[i]) {
									if(windows[i]) {
										placeableWindow = windows[i];
										break;
									}
								}
							}
							break;
						}
					}
					break;
				case KeyRelease:
					if(!mouseLock && windowAmt) {
						ev.xkey.window = selectedWindow;
						myfile << "Sent to window2 " << ev.xkey.keycode << "\n";
						XSendEvent(dpy, selectedWindow, true, KeyReleaseMask, &ev);
						break;
					}
					switch(ev.xkey.keycode) {
						case 38:
							key[0] = 0;
							break;
						case 40:
							key[1] = 0;
							break;
						case 25:
							key[2] = 0;
							break;
						case 39:
							key[3] = 0;
							break;
						case 65:
							key[4] = 0;
							break;
						case 50:
							key[5] = 0;
							break;
					}
					break;
				case MotionNotify: {
					mouseX = ev.xmotion.x;
					mouseY = ev.xmotion.y;
					if(!mouseLock && windowAmt) {
						ev.xmotion.window = selectedWindow;
						//myfile << "Motion Event " << "\n";
						XSendEvent(dpy, selectedWindow, true, PointerMotionMask, &ev);
						//myfile << "Finish Motion Event" << "\n";
						break;
					}
					if(ignoreNextMotion == true) {
						ignoreNextMotion = false;
						break;
					}
					XWindowAttributes winAttrs;
					XGetWindowAttributes(dpy, root, &winAttrs);
					if(winAttrs.width == 0)
						break;
					int relX = ev.xmotion.x-winAttrs.width/2;
					int relY = ev.xmotion.y-winAttrs.height/2;
					//myfile << "Motion notify \n" << relX << "\n" << relY << "\n";
					rotate_y += relX/600.0;
					rotate_x += relY/600.0;
					
					if(relX != 0 || relY != 0) {
						XWarpPointer(dpy, None, root, 0, 0, 0, 0, winAttrs.width/2, winAttrs.height/2);
						ignoreNextMotion = true;
					}
					break;
				}
				case ButtonPress: {
					myfile << "Button press " << ev.type << "\n";
					if(!mouseLock && windowAmt) {
						ev.xbutton.window = selectedWindow;
						XSendEvent(dpy, selectedWindow, true, ButtonPressMask, &ev);
						break;
					}
					double dist;
					Vector* dir = new Vector(0,0,1);
					double newY = dir->y * cos(-rotate_x) - sin(-rotate_x) * dir->z;
					double newZ = dir->y * sin(-rotate_x) + cos(-rotate_x) * dir->z;
					dir->y = newY;
					dir->z = newZ;
					double newX = dir->x * cos(rotate_y) - sin(rotate_y) * dir->z;
						   newZ = dir->x * sin(rotate_y) + cos(rotate_y) * dir->z;
					dir->x = newX;
					dir->z = newZ;
					Vector* rayPos = new Vector(pos->x, pos->y+1.6, pos->z);
					linePoints[0] = (GLfloat) rayPos->x;
					linePoints[1] = (GLfloat) rayPos->y;
					linePoints[2] = (GLfloat) rayPos->z;
					double step = 0;
					XWindowAttributes winAttrs;
					XGetWindowAttributes(dpy, xWin, &winAttrs);
					if(winAttrs.width == 0)
						break;
					GLfloat depth_comp;
					glReadPixels(
						winAttrs.width/2, winAttrs.height/2, 
						1, 1, 
						GL_DEPTH_COMPONENT,
						GL_FLOAT,
						&depth_comp);
					if(depth_comp == 1.0f)
						break;
					//float realDepth = (maxZ-minZ)*depth_comp + minZ;
					GLfloat clip_z = (depth_comp - 0.5f) * 2.0f;
					GLfloat realDepth = 2*maxZ*minZ/(clip_z*(maxZ-minZ)-(maxZ+minZ));
					
					rayPos->x = rayPos->x + dir->x * realDepth;
					rayPos->y = rayPos->y + dir->y * realDepth;
					rayPos->z = rayPos->z + dir->z * realDepth;
					linePoints2[0] = (GLfloat) rayPos->x;
					linePoints2[1] = (GLfloat) rayPos->y;
					linePoints2[2] = (GLfloat) rayPos->z;
					
					int i = 0;
					
					switch(ev.xbutton.button) {
						case 1:
							while(!w->hasBlock(rayPos)) {
								rayPos->x = rayPos->x + dir->x * realDepth * 0.01;
								rayPos->y = rayPos->y + dir->y * realDepth * 0.01;
								rayPos->z = rayPos->z + dir->z * realDepth * 0.01;
								i += 1;
								if(i > 10) break;
							}
							w->setBlock(rayPos, NULL);
							break;
						case 3:
							while(w->hasBlock(rayPos)) {
								rayPos->x = rayPos->x - dir->x * realDepth * 0.01;
								rayPos->y = rayPos->y - dir->y * realDepth * 0.01;
								rayPos->z = rayPos->z - dir->z * realDepth * 0.01;
								i += 1;
								if(i > 100) break;
							}
							if(i <= 100) {
								for(int i = 0; i < windowAmt; i++) {
									if(placeableWindow == windows[i]) {
										w->setBlock(rayPos, new Block(&textures[i]));
										break;
									}
								}
							}
							break;
					}
					break;
				}
				case ButtonRelease: {
					myfile << "Button release " << ev.type << "\n";
					if(!mouseLock && windowAmt) {
						ev.xbutton.window = selectedWindow;
						XSendEvent(dpy, selectedWindow, true, ButtonReleaseMask, &ev);
						break;
					}
					break;
				}
				default:
					// myfile << ev.type << "\n";
					myfile << "IDK " << ev.type << "\n";
					break;
			}
			myfile.close();
			myfile.open("test.txt", std::ios_base::app);
		}
		
		//sdelete &pixmap;
		//pixmap = XCompositeNameWindowPixmap(dpy, win2);
		
		//rotate_y += 0.0005;
		rotate_x = std::max(std::min(PI/2, rotate_x), -PI/2);
		time1 = curtime();
		
		double localSpeed = (time1-time2)*speed;
		if(localSpeed <= 0.003f)
			continue;
		vY -= localSpeed * 0.15;
		time2 = time1;
		/*
		if(time2-firstTime > 10000) {
			myfile.close();
			return 0;
		}*/
		
		
		//std::cout << vY << "\n";
		//pos->y = std::max(1.0, pos->y+vY);
		
		
		if(key[5] == 1) {
			localSpeed *= 1.5;
		}
		
		double dS = (key[1]-key[0]) * localSpeed;
		double dF = (key[3]-key[2]) * localSpeed;
		
		double cosY = cos(rotate_y);
		double sinY = sin(rotate_y);
		//std::cout << rotate_y << " " << cosY << " " << sinY << "\n";
		
		double dz = cosY * dF + sinY * dS;
		double dx = - sinY * dF + cosY * dS;
		
		Vector* v = pos->clone();
		
		
		
		for(double x = -0.3; x <= 0.3; x += 0.6) {
			v->x = pos->x + x + dx;
			if(w->hasBlock(v)) {
				int temp = (int) (pos->x + dx + x);
				if(x < 0) {
					dx = temp + 1 - x - pos->x;
				} else {
					dx = temp - x - pos->x;
				}
				break;
			}
		}
		v->x = pos->x;
		
		for(double z = -0.3; z <= 0.3; z += 0.6) {
			v->z = pos->z + z + dz;
			if(w->hasBlock(v)) {
				int temp = (int) (pos->z + dz + z);
				if(z < 0) {
					dz = temp + 1 - z - pos->z;
				} else {
					dz = temp - z - pos->z;
				}
				break;
			}
		}
		v->z = pos->z;
		double dy = vY * localSpeed;
		for(double y = 0.0; y <= 1.8; y += 1.8) {
			v->y = pos->y + y + dy;
			if(w->hasBlock(v)) {
				int temp = (int) (pos->y + dy + y);
				if(y < 1) {
					vY = 0.0f;
					dy = temp + 1 - y - pos->y;
				} else {
					dy = temp - y - pos->y;
					vY = 0.0f;
				}
				break;
			}
		}
		delete v;
		
		pos->x += dx;
		pos->z += dz;
		pos->y = 1.0f;//+= dy;
		if(pos->x > 32 || pos->x < 0 || pos->z > 32 || pos->z < 0 || pos->y < -10 || pos->y > 32)
			{
				pos->x = 16;
				pos->y = 16;
				pos->z = 16;
				vY = 0.0f;
			}
		
		if(key[4] == 1 && pos->y <= 1.1f ) {
			vY = 1.0f;
		}
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		
		glFrustum(minX, maxX, minY, maxY, minZ, maxZ);
		//glScalef(100.0f,100.0f,100.0f);
		if(!mouseLock)
			draw_cursor(map(mouseX, DISPLAY_WIDTH, 0, minX, maxX), map(mouseY, 0, DISPLAY_HEIGHT, minY, maxY), -minZ-0.000001, maxX, maxX);
		Texture* placeableTexture = &textures[0];
		for(int i = 1; i < windowAmt; i++) {
			if(placeableWindow == windows[i]) {
				placeableTexture = &textures[i];
				if(!mouseLock) {
					XWindowAttributes attrs;
					XGetWindowAttributes(dpy, placeableWindow, &attrs);
					
					double winX = map(attrs.x, DISPLAY_WIDTH, 0, minX, maxX);
					double winY = map(attrs.y, 0, DISPLAY_HEIGHT, minY, maxY);
					double mWinX = map(attrs.x+attrs.width, DISPLAY_WIDTH, 0, minX, maxX);
					double mWinY = map(attrs.y+attrs.height, 0, DISPLAY_HEIGHT, minY, maxY);
					
					double vertZ = -minZ-0.000001;
					int textureIndex = placeableTexture->textInt;
					glBindTexture(GL_TEXTURE_2D, textureIndex);
					glBegin(GL_POLYGON);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(winX, winY, vertZ);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f(mWinX, winY, vertZ);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f(mWinX, mWinY, vertZ);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(winX, mWinY, vertZ);
					glEnd();
				}
				break;
			}
		}
		Block* b = new Block(placeableTexture);
		Vector* blockPos = new Vector(1, -2, -3.5);
		glScalef(0.001f, 0.001f, 0.001f);
		b->render(w, blockPos);
		glScalef(1000.0f, 1000.0f, 1000.0f);
		delete blockPos;
		delete b;
		
		glRotatef( rotate_x * 180/PI, 1.0, 0.0, 0.0 );
		glRotatef( rotate_y * 180/PI, 0.0, 1.0, 0.0 );
		glTranslatef(-pos->x, -pos->y-1.6, -pos->z);
		
		w->render();
		/*
		glTranslatef(pos->x, pos->y+1.6, pos->z);
		glRotatef( -rotate_y * 180/PI, 0.0, 1.0, 0.0 );
		glRotatef( -rotate_x * 180/PI, 1.0, 0.0, 0.0 );
		*/
		if(placeableWindow != windows[0]) {
		}
		
		glFlush();
		glXSwapBuffers( dpy, glxWin );
	
	}
	
	exit( EXIT_SUCCESS );
}
