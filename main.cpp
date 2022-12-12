#include "include/raylib.h"
#include "include/raymath.h"

#include <string>
#include <random>
#include <vector>
#include <cassert>

struct WindowDimension{
	int width;
	int height;
};
const WindowDimension windowedDim = { 1600, 900 };
const WindowDimension fullscreenDim = { 1600, 900 };

struct Ball{
	Color color;
	Vector2 center;
	Vector2 velocity;
	float radius;
	float speed;
	bool isAttachedToPlayerPaddle;
};

struct Wall{
	Rectangle rect;
	Color color;

	Wall(){}
	Wall( Vector2 center, float width, float height ){
		rect.x = center.x - width/2;
		rect.y = center.y - height/2;
		rect.width = width;
		rect.height = height;
	}

	void move( Vector2 v ){
		rect.x += v.x;
		rect.y += v.y;
	}
};

float percentOf( float x, float t ){
	return x / 100.0f * t;
}

struct UI{
	enum Mode { GAME_SCOREBOARD, GAME_DEBUG, MAINMENU, MODECOUNT } mode;
	const char* modeNames[ Mode::MODECOUNT ] = { "GAME_SCOREBOARD", "GAME_DEBUG VIEW", "MAINMENU" };

	UI(){}
	UI( Mode initializingMode ){
		mode = initializingMode;
	}

	void toggleScoreboardMenuModes(){
		assert( mode == Mode::GAME_SCOREBOARD || mode == Mode::GAME_DEBUG );
		if( mode == Mode::GAME_SCOREBOARD ) mode = Mode::GAME_DEBUG;
		else if( mode == Mode::GAME_DEBUG ) mode = Mode::GAME_SCOREBOARD;
	}


	void drawGameUI( int rpScore, int lpScore, int screenWidth ){
		std::string rpScoreText = std::to_string( rpScore );
		std::string lpScoreText = std::to_string( lpScore );
		DrawText( rpScoreText.c_str(), 3*screenWidth/4, screenWidth/20, 60, RAYWHITE );
		DrawText( lpScoreText.c_str(), screenWidth/4, screenWidth/20, 60, WHITE );

		DrawText( "Press F11 to toggle fullscreen and G to toggle UI mode", 0, 0, 10, BLACK );
		DrawText( modeNames[ mode ], 0, 10, 10, GREEN );
	}

	void drawDebugUI( float targetedYPos, Ball ball, int rpScore, int lpScore ){
		DrawText( "Press F11 to toggle fullscreen and G to toggle UI mode", 0, 0, 10, BLACK );
		DrawText( modeNames[ mode ], 0, 10, 10, GREEN );

		std::string predAiPos = "Predicted AI y position: " + std::to_string( targetedYPos );
		std::string curBSpeed = "Current ball speed: " + std::to_string( ball.speed );
		std::string lefpScore = "Left player score: " + std::to_string( lpScore );
		std::string rigpScore = "Right player score: " + std::to_string( rpScore );
		
		DrawText( predAiPos.c_str(), 0, 20, 10, RAYWHITE );
		DrawText( curBSpeed.c_str(), 0, 30, 10, RAYWHITE );
		DrawText( lefpScore.c_str(), 0, 40, 10, RAYWHITE );
		DrawText( rigpScore.c_str(), 0, 50, 10, RAYWHITE );
		DrawFPS( 0, 60 );
	}

	void drawMenuUI( size_t currentSelection, std::vector<std::string>& menuOptions, int resX, int resY ){
		assert( menuOptions.size() > 0 );
		assert( resX > 0 );
		assert( resY > 0 );

		DrawText( std::to_string( currentSelection ).c_str(), 0, 0, 10, RED );
	
		// currentSelection wraparound logic
		if( currentSelection < 0 ) currentSelection = menuOptions.size() - 1;
		if( currentSelection >= menuOptions.size() ) currentSelection = 0;

		DrawText( std::to_string( currentSelection ).c_str(), 0, 10, 10, LIGHTGRAY );

		int xPos = percentOf( resX, 50.f );
		int yPos = percentOf( resY, 50.f );
		int fontSize = 40;
		std::string title = "PONG";

		// Add an indicator of current selection
		menuOptions[ currentSelection ] += "\t <";

		for( size_t i = 0; i < menuOptions.size(); ++i ){
			DrawText( menuOptions[i].c_str(), xPos, yPos + fontSize*i, fontSize, RAYWHITE );
		}

		DrawText( title.c_str(), xPos, 0, 100, ORANGE );
	}
};


struct PongGame{
	// World specific settings
	int courtWidth;
	int courtHeight;
	Wall topWall;
	Wall bottomWall;
	Wall leftCollider;
	Wall rightCollider;
	Wall leftPaddle;
	Wall rightPaddle;
	Ball ball;

	// Game specific settings
	bool gameStarted;
	int rightPlayerScore;
	int leftPlayerScore;
	float paddleSpeed = 500;
	float aiError = 0;
	float targetedYPos;
	float initialBallSpeed = 500;
	float ballSpeedIncreaseRate = 1.0001;

	// Colors used for debugging
	Color wallColor{ 160, 186, 168, 255 };
	Color colliderColor{ 152, 12, 13, 10 };
	Color paddleColor{ 250, 147, 89, 255 };
	Color selectedColor{ 230, 180, 110, 255 };
	Color ballColor = LIGHTGRAY;

	PongGame( int width, int height ){
		positionWalls( width, height );
		rightPlayerScore = 0;
		leftPlayerScore = 0;

		ball.radius = percentOf( courtWidth, 1.f );
		ball.velocity = Vector2{ 0, 0 };
		ball.speed = 0;
		ball.color = ballColor;

		leftPaddle.rect.x = percentOf( courtWidth, 2.f );
		leftPaddle.rect.width = percentOf( courtWidth, 1.f );
		leftPaddle.rect.height = percentOf( courtHeight, 15.f );
		leftPaddle.color = RAYWHITE;

		rightPaddle.rect.width = percentOf( courtWidth, 1.f );
		rightPaddle.rect.height = percentOf( courtHeight, 15.f );
		rightPaddle.rect.x = courtWidth - percentOf( courtWidth, 2.f ) - rightPaddle.rect.width;
		rightPaddle.color = RAYWHITE;

		topWall.color = wallColor;
		bottomWall.color = wallColor;
		leftCollider.color = colliderColor;
		rightCollider.color = colliderColor;
	}

	void drawShapes(){
		DrawLine( courtWidth/2, 0, courtWidth/2, courtHeight, topWall.color );
		DrawRectangleRec( topWall.rect, topWall.color );
		DrawRectangleRec( bottomWall.rect, bottomWall.color );
		DrawRectangleRec( leftCollider.rect, leftCollider.color );
		DrawRectangleRec( rightCollider.rect, rightCollider.color );
		DrawRectangleRec( leftPaddle.rect, leftPaddle.color );
		DrawRectangleRec( rightPaddle.rect, rightPaddle.color );
		DrawCircle( ball.center.x, ball.center.y, ball.radius, ball.color );
	}


	void positionWalls( int width, int height ){
		courtWidth = width;
		courtHeight = height;

		topWall.rect.x = 0;
		topWall.rect.y = 0;
		topWall.rect.width = courtWidth;
		topWall.rect.height = percentOf( courtHeight, 1.f );

		bottomWall.rect.x = 0;
		bottomWall.rect.y = courtHeight - percentOf( courtHeight, 1.f );
		bottomWall.rect.width = courtWidth;
		bottomWall.rect.height = percentOf( courtHeight, 1.f );

		leftCollider.rect.x = 0;
		leftCollider.rect.y = 0;
		leftCollider.rect.width = percentOf( courtWidth, 1.f );
		leftCollider.rect.height = courtHeight;

		rightCollider.rect.x = courtWidth - percentOf( courtWidth, 1.f );
		rightCollider.rect.y = 0;
		rightCollider.rect.width = (float)percentOf( courtWidth, 1.f );
		rightCollider.rect.height = courtHeight;
	}

	// move a paddle, with its y position constrained
	void movePaddle( Wall& paddle, float y ){
		float oldY = paddle.rect.y;
		paddle.move( Vector2{ 0, y } );
		if(
			paddle.rect.y < 0 ||
			paddle.rect.y + paddle.rect.height > courtHeight
		){
			paddle.rect.y = oldY;
		}
	}

	// todo: modify difficulty by adding error
	void moveOpponentPaddleTowardsY( Wall& opponentPaddle, float targetY, float speed ){
		float paddleCenterY = opponentPaddle.rect.y + 0.5*opponentPaddle.rect.height;
		if( paddleCenterY > targetY ) movePaddle( opponentPaddle, -speed );
		else{ movePaddle( opponentPaddle, speed ); }
	}

	void initMatch(){
		gameStarted = false;

		rightPaddle.rect.y = percentOf( courtHeight, 50 ) - rightPaddle.rect.height/2.f;
		leftPaddle.rect.y =  percentOf( courtHeight, 50 ) - leftPaddle.rect.height/2.f;

		ball.center.x = courtWidth/2.f;
		ball.center.y = courtHeight/2.f;
	}

	void step( float dt ){ if( gameStarted ){
		// comment out for a performance boost
		ball.velocity = Vector2Normalize( ball.velocity );

		ball.center.x += ball.velocity.x * ball.speed * dt;
		ball.center.y += ball.velocity.y * ball.speed * dt;

		// Handle score condition
		if( 
			CheckCollisionCircleRec( ball.center, ball.radius, leftCollider.rect )
		){
			rightPlayerScore++;
			initMatch();
		}
		if( 
			CheckCollisionCircleRec( ball.center, ball.radius, rightCollider.rect )
		){
			leftPlayerScore++;
			initMatch();
		}

		// Handle collisions
		if( 
			CheckCollisionCircleRec( ball.center, ball.radius, topWall.rect ) ||
			CheckCollisionCircleRec( ball.center, ball.radius, bottomWall.rect )
		){
			ball.velocity.y *= -1;
		}
		if( 
			CheckCollisionCircleRec( ball.center, ball.radius, rightPaddle.rect ) ||
			CheckCollisionCircleRec( ball.center, ball.radius, leftPaddle.rect )
		){
			ball.velocity.x *= -1;
		}

		// Move the opponent paddle to the guessed y position
		targetedYPos = calculateAiYPos( 0, ball, leftPaddle.rect.x + leftPaddle.rect.width );
		moveOpponentPaddleTowardsY( leftPaddle, targetedYPos, paddleSpeed*dt );

		// Make the game less boring over time
		ball.speed = powf( ball.speed, ballSpeedIncreaseRate );
	}}

	void startGame( float randNum1, float randNum2 ){
		float r1 = randNum1;
		float r2 = randNum2;
		if( r1 > r2 ){ float temp = r1; r2 = r1; r2 = temp; }
		ball.velocity = Vector2Normalize( Vector2{ r1, r2 } );
		ball.speed = initialBallSpeed;
		gameStarted = true;
	}

	float calculateAiYPos( float error, Ball& b, float paddleXPos ){
		// Determine y position of ball where its going to collide with the paddle
		float gradient = b.velocity.y / b.velocity.x;
		float c = b.center.y - gradient*b.center.x;
		float y = gradient*paddleXPos + c;

		// apply error
		y += error;
		return y;
	}
};

void toggleFullScreenAndResizeWorld( PongGame& world ){
	int newWidth; int newHeight;
	if( IsWindowFullscreen() ){
		newWidth = windowedDim.width;
		newHeight = windowedDim.height;
	}
	else{
		newWidth = fullscreenDim.width;
		newHeight = fullscreenDim.height;
	}
	SetWindowSize( newWidth, newHeight );
	world.positionWalls( newWidth, newHeight);

	ToggleFullscreen();
}

int main(){
	// Random number stuff
	std::mt19937 mt { std::random_device{}() };
	std::uniform_real_distribution<float> normalRandomNumber{ -1, 1 };

	// The actual game world
	PongGame world( windowedDim.width, windowedDim.height );
	world.initMatch();

	// UI specific settings
	UI gameUI( UI::Mode::MAINMENU );

	InitWindow( windowedDim.width, windowedDim.height, "Pong" );
	SetTargetFPS( 120 );
	HideCursor();

	auto mainMenuLoop = [ &gameUI, &world, &mt, &normalRandomNumber ](){
		std::vector< std::string > mainMenuOptions{
			"Start",
			"Exit",
		};
		static int currentChoice;

		// Toggle between fullscreen and windowed
		if( IsKeyPressed( KEY_F11 ) ){
			toggleFullScreenAndResizeWorld( world );
		}

		if( IsKeyPressed( KEY_DOWN ) ){
			currentChoice++;
		}
		if( IsKeyPressed( KEY_UP ) ){
			currentChoice--;
		}
		if( IsKeyPressed( KEY_ENTER ) ){
			switch ( currentChoice )
			{
			// Switch to the game
			case 0:
				gameUI = UI::Mode::GAME_SCOREBOARD;
				break;
			
			// Exit
			case 1:
				exit( 0 );
				break;

			default:
				assert( 0 && "Invalid option" );
				break;
			}
		}

		// Make the option cursor wrap around
		currentChoice %= mainMenuOptions.size();

		BeginDrawing();
			ClearBackground( BLACK );
			gameUI.drawMenuUI( currentChoice, mainMenuOptions, world.courtWidth, world.courtHeight );
		EndDrawing();

	};

	auto mainGameLoop = [ &gameUI, &world, &mt, &normalRandomNumber ](){
		float deltaTime = 1 / (float)GetFPS();

		// Boot player into the main menu when either player has scored 5 points for now
		if( world.leftPlayerScore >= 5 || world.rightPlayerScore >= 5 ) gameUI = UI::Mode::MAINMENU;

		if( IsKeyPressed( KEY_G ) ){
			gameUI.toggleScoreboardMenuModes();
		}

		// Toggle between fullscreen and windowed
		if( IsKeyPressed( KEY_F11 ) ){
			toggleFullScreenAndResizeWorld( world );
		}

		// Start the game
		if( IsKeyPressed( KEY_SPACE ) && !world.gameStarted ){
			float randNum1 = normalRandomNumber( mt );
			float randNum2 = normalRandomNumber( mt );
			world.startGame( randNum1, randNum2 );
		}

		// Player movement
		if( IsKeyDown( KEY_UP ) ){
			world.movePaddle( world.rightPaddle, -world.paddleSpeed*deltaTime );
		}
		if( IsKeyDown( KEY_DOWN ) ){
			world.movePaddle( world.rightPaddle, +world.paddleSpeed*deltaTime );
		}

		BeginDrawing();
			ClearBackground( BLACK );
			
			world.drawShapes();
			if( gameUI.mode == UI::Mode::GAME_SCOREBOARD ) gameUI.drawGameUI( world.rightPlayerScore, world.leftPlayerScore, world.courtWidth );
			if( gameUI.mode == UI::Mode::GAME_DEBUG ) gameUI.drawDebugUI( world.targetedYPos, world.ball, world.rightPlayerScore, world.leftPlayerScore );
		EndDrawing();

		world.step( deltaTime );
	};

	while ( !WindowShouldClose() ){

		/*

███╗░░██╗░█████╗░████████╗███████╗  ████████╗░█████╗░  ░██████╗███████╗██╗░░░░░███████╗██╗
████╗░██║██╔══██╗╚══██╔══╝██╔════╝  ╚══██╔══╝██╔══██╗  ██╔════╝██╔════╝██║░░░░░██╔════╝╚═╝
██╔██╗██║██║░░██║░░░██║░░░█████╗░░  ░░░██║░░░██║░░██║  ╚█████╗░█████╗░░██║░░░░░█████╗░░░░░
██║╚████║██║░░██║░░░██║░░░██╔══╝░░  ░░░██║░░░██║░░██║  ░╚═══██╗██╔══╝░░██║░░░░░██╔══╝░░░░░
██║░╚███║╚█████╔╝░░░██║░░░███████╗  ░░░██║░░░╚█████╔╝  ██████╔╝███████╗███████╗██║░░░░░██╗
╚═╝░░╚══╝░╚════╝░░░░╚═╝░░░╚══════╝  ░░░╚═╝░░░░╚════╝░  ╚═════╝░╚══════╝╚══════╝╚═╝░░░░░╚═╝
		┌─────────────────────────────── ∘°❉°∘ ────────────────────────────────┐
		| Probably should seperate the UI from the actual state of the program |
		└─────────────────────────────── °∘❉∘° ────────────────────────────────┘
		 */
		switch ( gameUI.mode ) {
		case UI::Mode::MAINMENU:
			mainMenuLoop();
			break;

		case UI::Mode::GAME_DEBUG:
		case UI::Mode::GAME_SCOREBOARD:
			mainGameLoop();
			break;

		case UI::Mode::MODECOUNT:
		default:
			assert( 0 ); // invalid state
			break;
		}
	}

	CloseWindow();
	return 0;
}