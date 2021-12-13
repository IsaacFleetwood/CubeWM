
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <iostream>
#include <thread>
#include <X11/keysym.h>
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
		GLuint textInt;
		char* name;
		
		Texture(char* name) : name(name) {}

};

static Texture* GLX_WRAPPER_CURRENT_TEXTURE;
static Display* dpy;
static bool started = false;

static GLuint wrapper_glx_set_texture(Texture* texture) {
	if(GLX_WRAPPER_CURRENT_TEXTURE == texture)
		return texture->textInt;
	if(!texture->textInt) {
		GLX_WRAPPER_CURRENT_TEXTURE = texture;
		SDL_Surface *surface;
		GLuint textureInt;
		surface = IMG_Load(texture->name);
		glGenTextures(1, &textureInt);
		texture->textInt = textureInt;
		glBindTexture(GL_TEXTURE_2D, textureInt);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		SDL_FreeSurface(surface);
		return textureInt;
	}
	return texture-> textInt;
	// ?????
	//glGenTextures(1, &(texture->glxPixmap));
	/*
	glBindTexture(GL_TEXTURE_2D, *(texture->glxPixmap));
	glXBindTexImageEXT(dpy, *(texture->glxPixmap), GLX_FRONT_EXT, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GLX_WRAPPER_CURRENT_TEXTURE = texture;*/
	
}

class World {
	public:
		
		Chunk* chunks[2][1][2];
		
		World();
		
		bool hasBlock(Vector* v);
		bool setBlock(Vector* v, Block* b);
		
		void render();
		
};

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
					GLuint index = wrapper_glx_set_texture(this->textures[dx]);
					this->renderRectangleX(dx, pos, index);
				}
			}
			tempVec->x = pos->x;
			for(int dy = 0; dy < 2; dy++) {
				tempVec->y = pos->y + (dy << 1)-1;
				if(!(w->hasBlock(tempVec))) {
					GLuint index = wrapper_glx_set_texture(this->textures[2|dy]);
					this->renderRectangleY(dy, pos, index);
				}
			}
			tempVec->y = pos->y;
			for(int dz = 0; dz < 2; dz++) {
				tempVec->z = pos->z + (dz << 1)-1;
				if(!(w->hasBlock(tempVec))) {
					GLuint index = wrapper_glx_set_texture(this->textures[4|dz]);
					this->renderRectangleZ(dz, pos, index);
				}
			}
			delete tempVec;
		}
		
		void renderRectangleX(int dx, Vector* pos, GLuint textureIndex) {
			int newX = pos->x + dx;
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glBegin(GL_POLYGON);
			//glColor3f(  dx,  1.0,  1.0 );
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(newX, pos->y, pos->z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(newX, pos->y, pos->z+1);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(newX, pos->y+1, pos->z+1);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(newX, pos->y+1, pos->z);
			glEnd();
		}
		
		void renderRectangleY(int dy, Vector* pos, GLuint textureIndex) {
			int newY = pos->y + dy;
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
			int newZ = pos->z + dz;
			glBindTexture(GL_TEXTURE_2D, textureIndex);
			glBegin(GL_POLYGON);
			//glColor3f(  1.0,  1.0,  dz );
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(pos->x, pos->y, newZ);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(pos->x+1, pos->y, newZ);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(pos->x+1, pos->y+1, newZ);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(pos->x, pos->y+1, newZ);
			glEnd();
		}
		
};

Block* BLOCK_GRASS = new Block(new Texture("grass.png"));
Block* BLOCK_WOOD = new Block(new Texture("planks_oak.png"));
Block* BLOCK_COBBLESTONE = new Block(new Texture("cobblestone.png"));
Block* BLOCK_DIRT = new Block(new Texture("dirt.png"));
Block* BLOCK_AIR = new Block();

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
	None
};


static Bool WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
	return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
int main( int argc, char *argv[] )
{
	Display *dpy;
	Window                xWin;
	
	dpy = XOpenDisplay( NULL );
	if ( dpy == NULL ) {
		printf( "Unable to open a connection to the X server\n" );
		exit( EXIT_FAILURE );
	}
	XEvent                event;
	XVisualInfo          *vInfo;
	GLXFBConfig          *fbConfigs;
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
	
	xWin = XCreateWindow( dpy, DefaultRootWindow(dpy), 1, 1, 2560, 1440,
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
	
	glXMakeContextCurrent( dpy, glxWin, glxWin, context );
	/*
	thread graphicsThread(updateGraphics, dpy, &xWin);
	
	while(!graphics_initialization_finished) {}
	*/
	
	// CirculateNotify | ConfigureNotify | CreateNotify | DestroyNotify | GravityNotify | MapNotify | MappingNotify | ReparentNotify | UnmapNotify | VisibilityNotify | 
	
	int eventMask = \
		KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	
	XSelectInput (dpy, xWin, eventMask);
  	
	/* Map the window to the screen, and wait for it to appear */
	//while(true) {}
	
	
	XEvent ev;
	XEvent lastEv;
	
	double rotate_y = 0;
	double rotate_x = 0;
	
	int key[6] = {0,0,0,0,0,0};
	Vector* pos = new Vector(8.0,1.0,8.0);
	
	glEnable(GL_DEPTH_TEST);
	
	glEnable(GL_TEXTURE_2D);
    glMatrixMode( GL_PROJECTION );
    
    const double PI = 3.141592653589;
    double fov = PI/2;
    double scl = 0.001;
    double speed = 0.01;
    double minX = -sin(fov/2) * scl;
    double maxX = -minX;
    double minZ = cos(fov/2) * scl;
    double maxZ = 1024.0f;
    double minY = minX * 9.0/16.0;
    double maxY = -minY;
    double vY = 0;
    World* w = new World();
    long double time1, time2;
    time2 = curtime();
    
    GLfloat linePoints[3];
    GLfloat linePoints2[3];
    
    while(1) {
    	glViewport(0, 0, 2560, 1440); 
      	while(XCheckMaskEvent (dpy, eventMask, &ev)) {
			switch(ev.type) {
				case KeyPress:
					//std::cout << "Key press " << ev.xkey.keycode << "\n";
					switch(ev.xkey.keycode) {
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
					}
					break;
				case KeyRelease:
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
					XWindowAttributes winAttrs;
					XGetWindowAttributes(dpy, xWin, &winAttrs);
					if(winAttrs.width == 0)
						break;
					int relX = ev.xmotion.x-winAttrs.width/2;
					int relY = ev.xmotion.y-winAttrs.height/2;
					rotate_y += relX/600.0;
					rotate_x += relY/600.0;
					if(relX != 0 || relY != 0) 
						XWarpPointer(dpy, None, xWin, 0, 0, 0, 0, winAttrs.width/2, winAttrs.height/2);
					break;
				}
				case ButtonPress: {
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
					std::cout << realDepth << " " << depth_comp << "\n";
					rayPos->x = rayPos->x + dir->x * realDepth;
					rayPos->y = rayPos->y + dir->y * realDepth;
					rayPos->z = rayPos->z + dir->z * realDepth;
					linePoints2[0] = (GLfloat) rayPos->x;
					linePoints2[1] = (GLfloat) rayPos->y;
					linePoints2[2] = (GLfloat) rayPos->z;
					std::cout << rayPos->x << " " << rayPos->y << " " << rayPos->z << "\n";
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
							if(i <= 100)
								w->setBlock(rayPos, BLOCK_DIRT);
							break;
					}
					break;
				}
				default:
					std::cout << ev.type << "\n";
					break;
			}
		}
		//rotate_y += 0.05;
		rotate_x = std::max(std::min(PI/2, rotate_x), -PI/2);
		time1 = curtime();
		double localSpeed = (time1-time2)*speed;
		time2 = time1;
		
		vY -= localSpeed * 0.025;
		
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
		for(double y = 0.0; y <= 1.8; y += 1.8) {
			v->y = pos->y + y + vY;
			if(w->hasBlock(v)) {
				int temp = (int) (pos->y + vY + y);
				if(y < 1) {
					vY = temp + 1 - y - pos->y;
				} else {
					vY = temp - y - pos->y;
				}
				break;
			}
		}
		delete v;
		
		pos->x += dx;
		pos->z += dz;
		pos->y += vY;
		
		if(key[4] == 1 && abs(vY) < 0.01 ) {
			vY = 0.1;
		}
		/*
		pos->y += (key[4]-key[5]) * localSpeed;
		if(w->hasBlock(pos)) {
			pos->y -= (key[4]-key[5]) * localSpeed;
		}*/
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		
		glFrustum(minX, maxX, minY, maxY, minZ, maxZ);
		
		glRotatef( rotate_x * 180/PI, 1.0, 0.0, 0.0 );
		glRotatef( rotate_y * 180/PI, 0.0, 1.0, 0.0 );
		glTranslatef(-pos->x, -pos->y-1.6, -pos->z);
		
		w->render();
    	glLineWidth(10);
		glBegin(GL_LINES);
		glVertex3fv(linePoints);
		glVertex3fv(linePoints2);
		glEnd();
		
		glFlush();
		glXSwapBuffers( dpy, glxWin );
	
	}
	
	exit( EXIT_SUCCESS );
}
