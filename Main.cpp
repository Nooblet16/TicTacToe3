#include <math.h>
#include <stdio.h>
#include <Windows.h>
#include <WindowsX.h>
#include <gl/GL.h>
#include "VectorMath.h"
#include "GUI.h"
#include "Mesh.h"
#include "Array3.h"
#include "Game.h"

static const float Pi = 3.14159265358979323846f;

//Background color
static const float bkColor[] = {.9f,.9f,.9f,0};

//Player mark colors
static const float markAlpha = .9f; //Alpha (opacity) value for transparency.
static const float markColor1[] = {1,0.1f,0.1f,markAlpha};
static const float markColor2[] = {0.3f,0.3f,1,markAlpha};

//Grid colors
static float gridFacetAlpha = .2f;
static float gridFacetColor[] = {0,0,0,gridFacetAlpha};
static float gridCurCellColor[] = {0,0,0,.2f};

//Color to highlight a winning combination on the grid.
static const float winAlpha = .7f;
static const float winColorAmbient[] = {2,2,0,winAlpha};
static const float winColorDiffuse[] = {1,1,0,winAlpha};

//Light properties: ambient, diffuse and specular components for Phong's lighting model implemented in OpenGL.
static const float lAmbient[] =  {.2f, .2f, .2f, 1.f};
static const float lDiffuse[] =  {.8f, .8f, .8f, 1.f};
static const float lSpecular[] = {.3f, .3f, .3f, 1.f};
static const float lPosition[] = {
	//The light's XYZ coordinates are set so that the light appears to shine 
	// from behind and above the viewer and slightly from the right.
	1,2,3,
	//Coordinate W (lPosition[3]) is set to 0 so the light is positioned at infinity.
	//In other words, this is a directional light.
	//Set this to 1 for a point light.
	0
};

//static const int ComputerThinkTime = 20; //Number of frames a computer player takes to "think".
static const int ComputerThinkTime = 10; //Number of frames a computer player takes to "think".

struct View
{
	float fov; //Horizontal field of view angle, in radians.
	float rotation[2]; //Model rotation angles, in degrees.
	float zoom; //Distance from the viewer to the model.
	float dir[3]; //View direction vector, calculated from angles.

	View(){
		fov = 1.f;
		rotation[0] = 0;
		rotation[1] = 0;
		zoom = 5.f;
		set(dir, 0,0,-1);
	}
	void calcViewDir()
	{
		//Calculate the direction viewer is looking from rotation angles.
		//Basically, convert it from polar to rectangular coordinates.
		set(dir, 0,0,-1);
		rotateX(dir, -rotation[0]*Pi/180);
		rotateY(dir,  rotation[1]*Pi/180);
	}
};



struct Application
{
	HWND window;
	HDC dc;
	HGLRC glrc; //OpenGL rendering context handle
	View view;
	POINT press; //Point in window where the LMB was pressed. Used to distinguish impresize clicks from short drags.
	POINT cursor; //Point in window where the mouse cursor is currently.
	Game game;
	GUI gui;
	
	float gridAnimScale;//Grid "expand" animation when starting the game.
	bool inSession; //Flag that is set when the game is currently playing.
	int playerTurn; //Index of the player whose turn is currently.
	int latestMark;//The index of the cell where a mark has just been put by a player. Used to animate the mark inside that cell.
	float markAnimScale;
	int thinkTimeout;//Delay to slow things down for computer players.

	Array3<int> sortedCells;//An array of grid cell indices, sorted in back-to-front order to render with correct transparency.
	int selection[3]; //The cell pointed at by a player's cursor.

	//OpenGL display lists for cube and sphere to speed up rendering.
	int cubeDisplayList;
	int sphereDisplayList;
	
	Application()
	{
		press.x = 0;
		press.y = 0;
		cursor.x = 0;
		cursor.y = 0;
		gridAnimScale = 0;
		inSession = false;
		playerTurn = 0;
		latestMark = -1;
		markAnimScale = 0;
		set(selection, -1,-1,-1);
		thinkTimeout = 0;
		
		if(! createWindow(800, 600)){
			return;
		}
		dc = GetDC(window);
		if(! initOpenGL()){
			MessageBox(0, "Failed to initialize OpenGL", "Tic Tac Toe", MB_OK|MB_ICONERROR);
			return;
		}
		createDisplayLists();
		
		game.reset(3, 3);
		resetSortedCells();
		view.calcViewDir();
		sortCellsBackToFront();
		
		gui.init();
		gui.setScreen(GUI::MainMenu);
		
		loop();
	}
	bool createWindow(int width, int height)
	{
		WNDCLASS c;
		ZeroMemory(&c, sizeof(c));
		c.lpszClassName = "TicTacToeGame";
		c.lpfnWndProc = DefWindowProc;
		c.hCursor = LoadCursor(NULL, IDC_ARROW);
		ATOM a = RegisterClass(&c);
		window = CreateWindow(
			c.lpszClassName, "Tic Tac Toe", WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
			0, 0, width, height, NULL, NULL, NULL, NULL);
		if(window){
			ShowWindow(window, SW_SHOWNORMAL);
		}
		return (window != 0);
	}
	bool initOpenGL()
	{
		PIXELFORMATDESCRIPTOR pfd;
		ZeroMemory(&pfd, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = 
			PFD_DRAW_TO_WINDOW | 
			PFD_SUPPORT_OPENGL |
			PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 16;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int format = ChoosePixelFormat(dc, &pfd);
		if(SetPixelFormat(dc, format, &pfd)){
			glrc = wglCreateContext(dc);
			if(glrc){
				if(! wglMakeCurrent(dc, glrc)){
					wglDeleteContext(glrc);
					glrc = 0;
				}
			}
		}
		return (glrc != 0);
	}
	void createDisplayLists()
	{
		cubeDisplayList = glGenLists(1);
		glNewList(cubeDisplayList, GL_COMPILE);
		drawCube();
		glEndList();
		sphereDisplayList = glGenLists(1);
		glNewList(sphereDisplayList, GL_COMPILE);
		drawSphere();
		glEndList();
	}
	void loop()
	{
		MSG msg;
		while(IsWindow(window)){
			while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)){
				DispatchMessage(&msg);
				if(! IsWindow(window)){
					break;
				}
				if(msg.message == WM_LBUTTONUP){
					//Click event happens on button up, 
					//because on button down we don't know yet 
					//if the player wants to click or drag.
					if(gui.getPlayerType(playerTurn) == 0){//Human player
						int dx = press.x - GET_X_LPARAM(msg.lParam);
						int dy = press.y - GET_Y_LPARAM(msg.lParam);
						if(dx*dx+dy*dy < 5*5){ //It's a click, not a drag.
							if(selection[0]>=0){ //Some grid cell is actually selected with the cursor.
								if(game.contents(selection) == 0){ //Selected cell is empty.
									putMark(selection[0], selection[1], selection[2], playerTurn);
									makeTurn();
								}
							}
						}
					}
				}
				if(msg.message == WM_LBUTTONDOWN){
					RECT clientRect;
					GetClientRect(window, &clientRect);
					int x = GET_X_LPARAM(msg.lParam);
					int y = GET_Y_LPARAM(msg.lParam);
					if(gui.screen == GUI::Game){
						press.x = x;
						press.y = y;
					}
					gui.onMouseDown(x, y, clientRect);
				}
				if(msg.message == WM_MOUSEMOVE){
					RECT clientRect;
					GetClientRect(window, &clientRect);
					int xPos = GET_X_LPARAM(msg.lParam);
					int yPos = GET_Y_LPARAM(msg.lParam);
					gui.onMouseMove(xPos, yPos, clientRect);
					if(gui.screen != GUI::MainMenu){
						if(msg.wParam & MK_LBUTTON){
							int xDelta = xPos - cursor.x;
							int yDelta = yPos - cursor.y;
							view.rotation[1] += 200.f * (float)xDelta / clientRect.right;
							view.rotation[0] += 200.f * (float)yDelta / clientRect.right;
							view.calcViewDir();
							sortCellsBackToFront();
						}
					}
					cursor.x = xPos;
					cursor.y = yPos;
				}
				if(msg.message == WM_MOUSEWHEEL){
					if(GET_WHEEL_DELTA_WPARAM(msg.wParam) < 0){
						view.zoom *= 1.1f;
					}else{
						view.zoom /= 1.1f;
					}
				}
			}
			process();
			refresh();
			Sleep(10);
		}
	}
	void makeTurn()
	{
		int gameState = game.checkGameState();
		if(gameState != 0){
			if(gameState != 3){
				game.markWinningLines();
			}
			gui.setScreen(GUI::Result);
			if(gameState == 1){
				gui.setWidgetText(GUI::ResultCaption, "Player 1 wins!");
			}
			if(gameState == 2){
				gui.setWidgetText(GUI::ResultCaption, "Player 2 wins!");
			}
			if(gameState == 3){
				gui.setWidgetText(GUI::ResultCaption, "Draw!");
			}
			return;
		}
		playerTurn = 3-playerTurn;//Alternate between 1 and 2
		set(selection, -1,-1,-1); //Invalidate selection
		thinkTimeout = ComputerThinkTime;
	}
	void process()
	{
		//Execute game logic and animation
		static int frame = 0;
		if(gui.screen == GUI::MainMenu){
			view.rotation[1] += .5f * sinf(frame/200.f);
			view.calcViewDir();
			sortCellsBackToFront();
		}
		if(gridAnimScale < 1.f){
			gridAnimScale = (gridAnimScale - 1.f) * .9f + 1.f;
		}
		if(markAnimScale < 1.f){
			markAnimScale = (markAnimScale - 1.f) * .5f + 1.f;
		}
		if(gui.screen != GUI::Game){
			inSession = false;
		}
		if(gui.screen == GUI::Game){
			if(! inSession){
				game.reset(gui.getGridSize(), gui.getToWin());
				resetSortedCells();
				view.calcViewDir();
				sortCellsBackToFront();
				gridFacetAlpha = .5f / game.size;//Denser grid shall have lower opacity to look consistent.
				gridFacetColor[3] = gridFacetAlpha;
				gridAnimScale = 0;
				inSession = true;
				playerTurn = 1;
				thinkTimeout = ComputerThinkTime;
			}
			if(gui.getPlayerType(playerTurn) != 0){//AI player type
				--thinkTimeout;
				if(thinkTimeout <= 0){
					int move = 0;
					if(gui.getPlayerType(playerTurn) == 1){
						//Random AI player
						move = game.randomMove();
					}else if(gui.getPlayerType(playerTurn) == 2){
						//Heuristic AI player
						move = game.heuristicMove(playerTurn);
					}else{
						//Minmax AI player
						move = game.minmaxMove(playerTurn);
					}
					latestMark = move;
					game.contents[move] = playerTurn;
					markAnimScale = 0;//Start mark "inflate" animation.
					makeTurn();
				}
			}
			gui.setWidgetVisible(GUI::GoPlayer1, playerTurn==1);
			gui.setWidgetVisible(GUI::GoPlayer2, playerTurn==2);
			if(gui.getPlayerType(playerTurn) == 0){//Human player
				getCellAtCursor();
			}
		}
		++frame;
	}
	void refresh()//Update the visuals.
	{
		RECT clientRect;
		GetClientRect(window, &clientRect);
		glViewport(0, 0, clientRect.right, clientRect.bottom);
		glClearColor(bkColor[0],bkColor[1],bkColor[2],0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		//The lights are set up with identity modelview matrix so they won't rotate together with the game model.
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		setupLighting();
		
		setupProjectionMatrix(clientRect.right, clientRect.bottom);
		setupViewMatrix();

		glEnable(GL_LIGHTING);
		glEnable(GL_NORMALIZE);//Renormalize normals so that scaled models will be lit properly.

		glDisable(GL_TEXTURE_2D);
		//Disable depth test to avoid Z-fighting between grid facets and lines.
		//We don't really need depth test because we sort objects manually for proper transparency.
		glDisable(GL_DEPTH_TEST);
		if(game.size > 0){
			drawGame();
		}
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		setupViewMatrixGUI(clientRect.right, clientRect.bottom);
		gui.draw();
		
		SwapBuffers(dc);//Force OpenGL to finish and present the framebuffer to the window.
	}
	void setupProjectionMatrix(int width, int height)
	{
		const float aspect = (float)width/height;
		const float zNear = .1f;
		const float zFar = view.zoom + 2.f;
		float L = -tan(view.fov/2)*zNear;
		float R = +tan(view.fov/2)*zNear;
		float B = -tan(view.fov/2)*zNear/aspect;
		float T = +tan(view.fov/2)*zNear/aspect;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(L, R, B, T, zNear, zFar);
	}
	void setupViewMatrixGUI(int width, int height)
	{
		const float aspect = (float)width/height;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glOrtho(-aspect, aspect, -1, 1, -1, 1);
	}
	void setupViewMatrix()
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0,0,-view.zoom);
		glScalef(gridAnimScale,gridAnimScale,gridAnimScale);
		glRotatef(view.rotation[0], 1,0,0);
		glRotatef(view.rotation[1], 0,1,0);
	}
	void drawGame()
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		//Disable lighting for wireframe objects like grid.
		glDisable(GL_LIGHTING);
		//drawGrid();

		//Enable lighting for solid objects - cubes, spheres and grid facets.
		glEnable(GL_LIGHTING);

		for(int a=0; a<game.contents.bufferSize(); a++){
			int i, j, k;
			int index = sortedCells[a];
			game.contents.getIndices(index, i, j, k);
			glPushMatrix();
			//Position the OpenGL modelview transform so that the object will fit inside the cell.
			glTranslatef(
				-1.f+2.f*(i+.5f)/game.size,
				-1.f+2.f*(j+.5f)/game.size,
				-1.f+2.f*(k+.5f)/game.size);
			const float scale = 1.f/game.size;
			glScalef(scale, scale, scale);
			drawGridFacet(i,j,k);
			// Highlight the user selected cell
			if(i==selection[0] && j==selection[1] && k==selection[2]){
				glMaterialfv(GL_FRONT, GL_AMBIENT, gridCurCellColor);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, gridCurCellColor);
				glCallList(cubeDisplayList);
			}
			int item = game.contents(i,j,k);//Look up the grid cell contents.
			if(item){//The cell has an item in it - a player's mark.
				float scale = (item==1) ? .5f : .6f;
				if(index == latestMark){//Animate the mark just put by a player.
					scale *= markAnimScale;
				}
				glScalef(scale,scale,scale);
				if(item == 1){
					glMaterialfv(GL_FRONT, GL_AMBIENT, markColor1);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, markColor1);
					glCallList(cubeDisplayList);
				}
				if(item == 2){
					glMaterialfv(GL_FRONT, GL_AMBIENT, markColor2);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, markColor2);
					glCallList(sphereDisplayList);
				}
				if(game.winning(i,j,k)){//This cell is a part of a winning combination - highlight it.
					glScalef(1.2f, 1.2f, 1.2f);
					glMaterialfv(GL_FRONT, GL_AMBIENT, winColorAmbient);
					glMaterialfv(GL_FRONT, GL_DIFFUSE, winColorDiffuse);
					glCallList(item==1 ? cubeDisplayList : sphereDisplayList);
				}
			}
			glPopMatrix();//Restore the previous modelview transform.
		}
	}
	void drawGridFacet(int i, int j, int k)
	{
		glMaterialfv(GL_FRONT, GL_AMBIENT, gridFacetColor);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, gridFacetColor);
		glBegin(GL_TRIANGLES);
			for(int f=0; f<6; f++){
				//Skip outer facets of the grid.
				if(f == 0 && i == 0) continue;
				if(f == 1 && i == game.size-1) continue;
				if(f == 3 && k == 0) continue;
				if(f == 2 && k == game.size-1) continue;
				if(f == 4 && j == 0) continue;
				if(f == 5 && j == game.size-1) continue;
				//Rectangular face is rendered as 2 triangles, 6 vertices total.
				glNormal3f(-cubeFacetNormals[f][0],-cubeFacetNormals[f][1],-cubeFacetNormals[f][2]);
				glVertex3fv(cubeVerts[cubeFacets[f][2]]);
				glVertex3fv(cubeVerts[cubeFacets[f][1]]);
				glVertex3fv(cubeVerts[cubeFacets[f][0]]);
				glVertex3fv(cubeVerts[cubeFacets[f][0]]);
				glVertex3fv(cubeVerts[cubeFacets[f][3]]);
				glVertex3fv(cubeVerts[cubeFacets[f][2]]);
			}
		glEnd();

		//Draw grid cell's edges as lines.
		glNormal3f(0,0,0);
		glBegin(GL_LINES);
		for(int e=0; e<12; e++){
			glVertex3fv(cubeVerts[cubeEdges[e][0]]);
			glVertex3fv(cubeVerts[cubeEdges[e][1]]);
		}
		glEnd();
	}
	void drawCube()
	{
		glBegin(GL_TRIANGLES);
			for(int f=0; f<6; f++){
				//Rectangular face is rendered as 2 triangles, 6 vertices total.
				glNormal3fv(cubeFacetNormals[f]);
				glVertex3fv(cubeVerts[cubeFacets[f][0]]);
				glVertex3fv(cubeVerts[cubeFacets[f][1]]);
				glVertex3fv(cubeVerts[cubeFacets[f][2]]);
				glVertex3fv(cubeVerts[cubeFacets[f][2]]);
				glVertex3fv(cubeVerts[cubeFacets[f][3]]);
				glVertex3fv(cubeVerts[cubeFacets[f][0]]);
			}
		glEnd();
	}
	void drawSphere()
	{
		glBegin(GL_TRIANGLES);
		for(int t=0; t<nSphereTris; t++){
			for(int i=0; i<3; i++){
				//Since vertices lie on a unit radius sphere, their coordinates equal to their normals'.
				glNormal3fv(sphereVerts[sphereTris[t][i]]);
				glVertex3fv(sphereVerts[sphereTris[t][i]]);
			}
		}
		glEnd();
	}
	//Setup OpenGL's lighting model parameters.
	void setupLighting()
	{
		glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDiffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lSpecular);
		glLightfv(GL_LIGHT0, GL_POSITION, lPosition);
		glEnable(GL_LIGHT0);

		//All objects have the same specular component, therefore setting it here.
		//Material colors will be set for each object individually prior to rendering that object.
		float mSpecular[] = {1,1,1,1};
		glMaterialfv(GL_FRONT, GL_SPECULAR, mSpecular);
		glMaterialf(GL_FRONT, GL_SHININESS, 20);
	}
	//In order to have correct transparency, objects must be sorted in back-to-front order relative to the viewer.
	//Since all objects in this game are located inside a rectangular grid, it is sufficient to sort grid cells.
	void sortCellsBackToFront()
	{
		sortCells(sortedCells.buffer, sortedCells.bufferSize());
	}
	//Simple recursive qucksort implementation for grid cells.
	void sortCells(int* v, int n)
	{
		if(n < 2){
			return;
		}
		int P = n/2;//Initialize pivot point.
		const int pivot = v[P];
		int L = 0;
		int R = n-1;
		while(true){
			while(L<=R && !isCellFurther(pivot, v[L])){
				++L;
			}
			while(L<=R && !isCellFurther(v[R], pivot)){
				--R;
			}
			if(L < R){
				//Swap v[L] and v[R]
				int x = v[L]; v[L] = v[R]; v[R] = x;
				L++;
				R--;
			}
			if(R < L){
				break;
			}
		}
		//if(R != L-1){
		//	printf("quickSort: R != L-1\n");
		//}
		if(P < L){
			//Swap v[R] with v[P]
			int x = v[R]; v[R] = v[P]; v[P] = x;
			R--;
		}else{
			//Swap v[L] with v[P]
			int x = v[L]; v[L] = v[P]; v[P] = x;
			L++;
		}
		int nL = R + 1;
		int nR = n - L;
		//Recurse into smaller partition first, to minimize stack depth.
		if(nL < nR){
			if(nL > 1){
				sortCells(v, nL);
			}
			if(nR > 1){
				sortCells(v+L, nR);
			}
		}else{
			if(nR > 1){
				sortCells(v+L, nR);
			}
			if(nL > 1){
				sortCells(v, nL);
			}
		}
	}
	//Comparison function for sorting grid cells in back-to-front order for correct transparency.
	//Return true is cell1 is further from the viewer than cell2.
	bool isCellFurther(int cell1, int cell2)
	{
		int x1, y1, z1;
		int x2, y2, z2;
		game.contents.getIndices(cell1, x1,y1,z1);
		game.contents.getIndices(cell2, x2,y2,z2);
		//Distances are calculated as dot products of view direction by a point representing the cell.
		float dist1 = x1 * view.dir[0] + y1 * view.dir[1] + z1 * view.dir[2];
		float dist2 = x2 * view.dir[0] + y2 * view.dir[1] + z2 * view.dir[2];
		return dist1 > dist2;
	}
	void resetSortedCells()
	{
		sortedCells.allocate(game.size);
		for(int i=0; i<sortedCells.bufferSize(); i++){
			sortedCells[i] = i;
		}
	}
	void getRayThroughCursor(float p[3], float v[3])
	{
		RECT c;
		GetClientRect(window, &c);
		v[0] = tan(view.fov/2) * view.zoom * (cursor.x*2 - c.right)  / c.right;
		v[1] = tan(view.fov/2) * view.zoom * (c.bottom - cursor.y*2) / c.right;
		v[2] = -view.zoom;
		rotateX(v, -view.rotation[0]*Pi/180);
		rotateY(v,  view.rotation[1]*Pi/180);
		p[0] = -view.zoom * view.dir[0];
		p[1] = -view.zoom * view.dir[1];
		p[2] = -view.zoom * view.dir[2];
	}
	void getCellAtCursor()
	{
		float p[3], v[3], h[3];
		getRayThroughCursor(p, v);
		set(selection, -1,-1,-1);//Invalidate selection.
		float tBest = FLT_MAX;
		for(int a=0; a<3; a++){
			if(a == maxDim(view.dir)){
				continue;
			}
			for(int j=0; j<=game.size; j++){
				float plane[4] = {0,0,0,0};
				plane[a] = 1;
				plane[3] = -1.f+2.f*j/game.size;
				if(p[a] < plane[3] && j==0){
					continue;
				}
				if(p[a] > plane[3] && j==game.size){
					continue;
				}
				float t = rayHitPlane(p, v, plane);
				if(t > 0){
					mulAdd(p, v, t, h);
					int b = (a+1)%3;
					int c = (a+2)%3;
					if(h[b]>-1.f && h[b]<1.f && h[c]>-1.f && h[c]<1.f){//Inside the cube?
						if(t < tBest){
							tBest = t;
							selection[a] = p[a] < plane[3] ? j-1 : j;
							selection[b] = (int)((h[b]+1.f)/2.f*game.size);
							selection[c] = (int)((h[c]+1.f)/2.f*game.size);
						}
					}
				}
			}
		}
	}
	void putMark(int i, int j, int k, int player)
	{
		latestMark = game.contents.index(i,j,k);
		game.contents(i,j,k) = playerTurn;
		markAnimScale = 0;//Start mark "inflate" animation.
	}
};


void main()
{
	Application();
}
