#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <cstdlib>
#include <string>
#include "math.h"

#include "ExtraFuncs.h"

// template this
std::vector<std::vector<float>> GenerateEmptyArray(int width, int height)
{
	std::vector<std::vector<float>> generatedVector;

	for (int row = 0; row < height; row++)
	{
		std::vector<float> tempEmpty;
		for (int column = 0; column < width; column++)
		{
			tempEmpty.push_back(0.f);
		}
		generatedVector.push_back(tempEmpty);
	}

	return generatedVector;
}

std::vector<std::vector<float>> GenerateWhiteNoise(int width, int height)
{
	int completedTiles = 0;

	srand(static_cast<unsigned int>(time(NULL)));
	std::vector<std::vector<float>> noise = GenerateEmptyArray(width, height);

	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			noise[row][column] = (float)RandomDouble(0.f, 1.f);
			completedTiles++;
			double percentComplete = (double)completedTiles / ((double)width*(double)height);
			std::cout << int(percentComplete * 100) << "%" << std::endl;
		}
	}

	return noise;
}

std::vector<std::vector<float>> GenerateSmoothNoise(std::vector<std::vector<float>> baseNoise, int octave)
{
	int completedTiles = 0;

	int width = baseNoise.size();
	int height = baseNoise[0].size();

	std::vector<std::vector<float>> smoothNoise = GenerateEmptyArray(width, height);

	int samplePeriod = 1 << octave; // calculates 2 ^ k
	float sampleFrequency = 1.0f / samplePeriod;

	for (int row = 0; row < height; row++)
	{
		//calculate the horizontal sampling indices
		int sample_i0 = (row / samplePeriod) * samplePeriod;
		int sample_i1 = (sample_i0 + samplePeriod) % width; //wrap around
		float horizontal_blend = (row - sample_i0) * sampleFrequency;

		for (int column = 0; column < width; column++)
		{
			//calculate the vertical sampling indices
			int sample_j0 = (column / samplePeriod) * samplePeriod;
			int sample_j1 = (sample_j0 + samplePeriod) % height; //wrap around
			float vertical_blend = (column - sample_j0) * sampleFrequency;

			//blend the top two corners
			float top = Interpolate(baseNoise[sample_i0][sample_j0], baseNoise[sample_i1][sample_j0], horizontal_blend);
			
			//blend the bottom two corners
			float bottom = Interpolate(baseNoise[sample_i0][sample_j1], baseNoise[sample_i1][sample_j1], horizontal_blend);

			//final blend
			smoothNoise[row][column] = Interpolate(top, bottom, vertical_blend);

			completedTiles++;
			double percentComplete = (double)completedTiles / ((double)width*(double)height);
			std::cout << int(percentComplete * 100) << "%" << std::endl;
		}
	}

	return smoothNoise;
}

std::vector<std::vector<float>> GeneratePerlinNoise(std::vector<std::vector<float>> baseNoise, int octaveCount)
{
	int completedTiles = 0;

	int width = baseNoise.size();
	int height = baseNoise[0].size();

	std::vector<std::vector<std::vector<float>>> smoothNoise = std::vector<std::vector<std::vector<float>>>(octaveCount);

	float persistance = 0.5f;

	//generate smooth noise
	for (int i = 0; i < octaveCount; i++)
	{
		smoothNoise[i] = GenerateSmoothNoise(baseNoise, i);

		completedTiles++;
		double percentComplete = (double)completedTiles / ((double)width*(double)height);
		std::cout << int(percentComplete * 100) << "%" << std::endl;
	}

	std::vector<std::vector<float>> perlinNoise = GenerateEmptyArray(width, height);
	float amplitude = 1.0f;
	float totalAmplitude = 0.0f;

	//blend noise together
	completedTiles = 0;
	for (int octave = octaveCount - 1; octave >= 0; octave--)
	{
		amplitude *= persistance;
		totalAmplitude += amplitude;

		for (int row = 0; row < height; row++)
		{
			for (int column = 0; column < width; column++)
			{
				perlinNoise[row][column] += smoothNoise[octave][row][column] * amplitude;

				completedTiles++;
				double percentComplete = (double)completedTiles / ((double)width*(double)height) / octaveCount;
				std::cout << int(percentComplete*100) << "%" << std::endl;
			}
		}
	}

	//normalisation
	completedTiles = 0;
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			perlinNoise[row][column] /= totalAmplitude;

			completedTiles++;
			double percentComplete = (double)completedTiles / ((double)width*(double)height);
			std::cout << int(percentComplete * 100) << "%" << std::endl;
		}
	}

	return perlinNoise;
}

void processCommands(std::vector<std::vector<float>>& PerlinNoise, bool& shouldRender, bool& shouldSave)
{
	while(true)
	{
		std::string sIn;
		std::cout << "Enter command (or 'help'): ";
		std::cin >> sIn;
		std::transform(sIn.begin(), sIn.end(), sIn.begin(), ::tolower);
		if (sIn == "adjust")
		{
			std::string inCon, inBright;
			std::cout << "Contrast modifier: ";
			std::cin >> inCon;
			std::cout << "Brightness modifier: ";
			std::cin >> inBright;
			float con = std::stof(inCon), bright = std::stof(inBright);

			int completedTiles = 0;
			for (int row = 0; row < PerlinNoise.size(); row++)
			{
				for (int column = 0; column < PerlinNoise[0].size(); column++)
				{
					//f(x) = a(x - 128) + 128 + b, where a = contrast and b = additional brightness
					//a > 1 more contrast, a < 1 less contrast
					//result clamped to range of 0 - 255
					//divide by 255 to get it back to a percentage before putting back in vector
					float factor = (259.0 * (con + 255.0)) / (255.0 * (259.0 - con));
					float x = PerlinNoise[row][column] * 255.f;
					float newValue = TruncateRGB((factor * (x - 128) + 128 + bright));

					PerlinNoise[row][column] = newValue/255;

					completedTiles++;
					double percentComplete = (double)completedTiles / ((double)PerlinNoise[0].size()*(double)PerlinNoise.size());
					std::cout << int(percentComplete * 100) << "%" << std::endl;
				}
			}
			shouldRender = true;
		}
		else if (sIn == "save")
		{
			shouldSave = true;
		}
		else if (sIn == "terrain")
		{

		}
		else if (sIn == "help")
		{
			std::cout << "Available commands\n'adjust'\n'save'\n'help'" << std::endl;
		}
	}
}

int main() {
	const unsigned int FPS = 0; //The desired FPS. (The number of updates each second) or 0 for uncapped.
	float timescale = 1.f;

	std::string inWidth, inHeight;
	std::cout << "Enter Perlin Noise Width: ";
	std::cin >> inWidth;
	std::cout << "Enter Perlin Noise Height: ";
	std::cin >> inHeight;

	int MapWidth = std::stoi(inWidth);
	int MapHeight = std::stoi(inHeight);

	const unsigned int BASE_DISPLAY_WIDTH = 512;
	const unsigned int BASE_DISPLAY_HEIGHT = 512;

	const sf::Vector2f BorderSize(50.f, 50.f);

	sf::Vector2f TileSize = sf::Vector2f(BASE_DISPLAY_WIDTH /MapWidth, BASE_DISPLAY_HEIGHT /MapHeight);

	const unsigned int SCREEN_WIDTH = BASE_DISPLAY_HEIGHT + BorderSize.x;
	const unsigned int SCREEN_HEIGHT = BASE_DISPLAY_HEIGHT + BorderSize.y;

	// TEST PERLIN NOISE GENERATION
	std::vector<std::vector<float>> WhiteNoise = GenerateWhiteNoise(MapWidth, MapHeight);
	//std::vector<std::vector<float>> SmoothNoise = GenerateSmoothNoise(WhiteNoise, 4);
	std::vector<std::vector<float>> PerlinNoise = GeneratePerlinNoise(WhiteNoise, 5);
	// TEST PERLIN NOISE GENERATION

	srand(static_cast<unsigned int>(time(NULL)));

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;

	std::string windowName = "Perlin Noise Generation";
	sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), (windowName + " " + GetEnvironmentBit()), sf::Style::Default, settings);
	window.setFramerateLimit(FPS);

	sf::Color backgroundColour = sf::Color(255, 0, 0);

	sf::Font ArialFont;
	ArialFont.loadFromFile("C:/Windows/Fonts/arial.ttf");

	// Convert to object
	const sf::Vector2f PlayerSize = sf::Vector2f(50, 50);
	float playerMoveSpeed = 256.f;
	const sf::Vector2i StartingTile = sf::Vector2i(4,4);
	sf::RectangleShape Player = sf::RectangleShape(PlayerSize);
	Player.setFillColor(sf::Color::Black);
	//Player.setOrigin(Player.getGlobalBounds().width / 2, Player.getGlobalBounds().height / 2);
	Player.setPosition(StartingTile.x*TileSize.x, StartingTile.y*TileSize.y);

	sf::Text PlayerCoordinates;
	PlayerCoordinates.setFont(ArialFont);
	PlayerCoordinates.setPosition(sf::Vector2f(0.f, 0.f));

	sf::RenderTexture rt;
	rt.create(MapWidth, MapHeight);
	rt.clear(backgroundColour);

	sf::Vector2i tilePositionLastFrame;
	bool paused = false;
	sf::Event ev;
	sf::Clock clock;


	bool shouldRender = true;
	bool shouldSave = false;

	std::thread t1(processCommands, std::ref(PerlinNoise), std::ref(shouldRender), std::ref(shouldSave));
	t1.detach();

	while (window.isOpen()) {
		float deltaTime = clock.restart().asSeconds();
		deltaTime *= timescale;
		float fps = 1.f / deltaTime;

		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed)
				window.close();

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
				window.close();
			}
		}

		if (shouldRender)
		{
			rt.clear(sf::Color::White);
			window.clear(sf::Color(41, 128, 185));

			for (int row = 0; row < MapHeight; row++)
			{
				for (int column = 0; column < MapWidth; column++)
				{
					sf::RectangleShape testTile = sf::RectangleShape(TileSize);

					sf::Color colour = sf::Color(PerlinNoise[row][column] * 255, PerlinNoise[row][column] * 255, PerlinNoise[row][column] * 255);

					testTile.setFillColor(colour);

					testTile.setPosition(sf::Vector2f(column, row));
					rt.draw(testTile);

					testTile.setPosition(sf::Vector2f((column*TileSize.x)+BorderSize.x/2, (row*TileSize.y)+BorderSize.y/2));
					window.draw(testTile);
				}
			}
			shouldRender = false;
		}
		
		window.display();

		if (shouldSave)
		{
			sf::Image temp = rt.getTexture().copyToImage();
			temp.flipVertically();
			temp.saveToFile("PerlinNoise.png");
			std::cout << "Saved!" << std::endl;
			shouldSave = false;
		}
	}
}