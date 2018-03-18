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

std::vector<std::vector<float>> GenerateWhiteNoise(unsigned int width, unsigned int height)
{
	srand(static_cast<unsigned int>(time(NULL)));
	std::vector<std::vector<float>> noise;

	for (unsigned int row = 0; row < height; row++)
	{
		std::vector<float> tempNoiseEntry;
		for (unsigned int column = 0; column < width; column++)
		{
			tempNoiseEntry.push_back((float)RandomDouble(0.f, 1.f));
		}
		noise.push_back(tempNoiseEntry);
	}
	return noise;
}

std::vector<std::vector<float>> GenerateSmoothNoise(std::vector<std::vector<float>> baseNoise, unsigned int octave)
{
	size_t width = baseNoise.size();
	size_t height = baseNoise[0].size();

	std::vector<std::vector<float>> smoothNoise;

	int samplePeriod = 1 << octave; // calculates 2 ^ k
	float sampleFrequency = 1.0f / samplePeriod;

	for (int row = 0; row < height; row++)
	{
		//calculate the horizontal sampling indices
		int sample_i0 = (row / samplePeriod) * samplePeriod;
		int sample_i1 = (sample_i0 + samplePeriod) % width; //wrap around
		float horizontal_blend = (row - sample_i0) * sampleFrequency;

		std::vector<float> tempSmoothNoiseEntry;
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
			tempSmoothNoiseEntry.push_back(Interpolate(top, bottom, vertical_blend));
		}
		smoothNoise.push_back(tempSmoothNoiseEntry);
	}

	return smoothNoise;
}

std::vector<std::vector<float>> GeneratePerlinNoise(std::vector<std::vector<float>> baseNoise, unsigned int octaveCount)
{
	size_t width = baseNoise.size();
	size_t height = baseNoise[0].size();

	std::vector<std::vector<std::vector<float>>> smoothNoise = std::vector<std::vector<std::vector<float>>>(octaveCount);

	float persistance = 0.5f;
	
	//generate smooth noise
	for (unsigned int i = 0; i < octaveCount; i++)
	{
		smoothNoise[i] = GenerateSmoothNoise(baseNoise, i);
	}

	std::vector<std::vector<float>> perlinNoise;
	float amplitude = 1.0f;
	float totalAmplitude = 0.0f;

	//blend noise together
	for (int octave = octaveCount - 1; octave >= 0; octave--)
	{
		amplitude *= persistance;
		totalAmplitude += amplitude;

		for (int row = 0; row < height; row++)
		{
			std::vector<float> tempPerlinNoiseEntry;
			for (int column = 0; column < width; column++)
			{
				// Check to see if this is the first loop and an entry needs to be created first
				if (octave == octaveCount - 1)
					tempPerlinNoiseEntry.push_back(smoothNoise[octave][row][column] * amplitude);
				else
					perlinNoise[row][column] += smoothNoise[octave][row][column] * amplitude;
			}
			// Check to see if this is the first loop and an entry needs to be created first
			if (octave == octaveCount - 1)
				perlinNoise.push_back(tempPerlinNoiseEntry);
		}
	}

	//normalisation
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			perlinNoise[row][column] /= totalAmplitude;
		}
	}

	return perlinNoise;
}

std::vector<std::vector<sf::Color>> ConvertToRGBVector(std::vector<std::vector<float>> baseMap)
{
	std::vector<std::vector<sf::Color>> RGBVector;

	for (int row = 0; row < baseMap.size(); row++)
	{
		std::vector<sf::Color> newRGBVectorEntry;
		for (int column = 0; column < baseMap[0].size(); column++)
		{
			newRGBVectorEntry.push_back(FloatToColour(baseMap[row][column]));
		}
		RGBVector.push_back(newRGBVectorEntry);
	}
	return RGBVector;
}

void processCommands(std::vector<std::vector<sf::Color>>& PerlinNoise, bool& shouldRender, bool& shouldSave)
{
	while(true)
	{
		std::string sIn;
		std::cout << "Enter command(s) (or 'help'): ";
		std::cin >> sIn;
		std::transform(sIn.begin(), sIn.end(), sIn.begin(), ::tolower);
		if (sIn == "adjust")
		{
			float con, bright;
			std::cout << "Contrast modifier: ";
			std::cin >> con;
			std::cout << "Brightness modifier: ";
			std::cin >> bright;

			for (int row = 0; row < PerlinNoise.size(); row++)
			{
				for (int column = 0; column < PerlinNoise[0].size(); column++)
				{
					//f(x) = a(x - 128) + 128 + b, where a = contrast and b = additional brightness
					//a > 1 more contrast, a < 1 less contrast
					//result clamped to range of 0 - 255
					//divide by 255 to get it back to a percentage before putting back in vector
					float  factor = (259.f * (con + 255.f)) / (255.f * (259.f - con));
					float x = PerlinNoise[row][column].r;
					float newValue = TruncateRGB((factor * (x - 128.f) + 128.f + bright));

					PerlinNoise[row][column] = FloatToColour(newValue/255);
				}
			}
			shouldRender = true;
		}
		else if (sIn == "save")
		{
			shouldSave = true;
		}
		else if (sIn == "filter")
		{
			for (int row = 0; row < PerlinNoise.size(); row++)
			{
				for (int column = 0; column < PerlinNoise[0].size(); column++)
				{
					// Gets the XOR of the generated greyscale colour and a predefined filter colour
					int XOR_Dec = RGBToDec(PerlinNoise[row][column]) ^ RGBToDec(sf::Color(127, 127, 254));
					PerlinNoise[row][column] = RGBFromDec(XOR_Dec);
				}
			}
			shouldRender = true;
		}
		else if (sIn == "help")
		{
			std::cout << "Available commands\n'adjust'\n'filter'\n'save'\n'help'" << std::endl;
		}
	}
}

int main() {
	const unsigned int FPS = 0; //The desired FPS. (The number of updates each second) or 0 for uncapped.
	float timescale = 1.f;

	unsigned int MapWidth, MapHeight;
	std::cout << "Enter Perlin Noise Width: ";
	std::cin >> MapWidth;
	std::cout << "Enter Perlin Noise Height: ";
	std::cin >> MapHeight;

	const unsigned int BASE_DISPLAY_WIDTH = 512;
	const unsigned int BASE_DISPLAY_HEIGHT = 512;

	const sf::Vector2f BorderSize(50.f, 50.f);
	
	sf::Vector2f TileSize = sf::Vector2f((float)BASE_DISPLAY_WIDTH / (float)MapWidth, (float)BASE_DISPLAY_HEIGHT / (float)MapHeight);

	const unsigned int SCREEN_WIDTH = BASE_DISPLAY_HEIGHT + (unsigned int)BorderSize.x;
	const unsigned int SCREEN_HEIGHT = BASE_DISPLAY_HEIGHT + (unsigned int)BorderSize.y;
	
	// GENERATE PERLIN NOISE
	std::vector<std::vector<float>> WhiteNoise = GenerateWhiteNoise(MapWidth, MapHeight);
	std::vector<std::vector<float>> PerlinNoise = GeneratePerlinNoise(WhiteNoise, 5);
	std::vector<std::vector<sf::Color>> RGBPerlinNoise = ConvertToRGBVector(PerlinNoise);
	// GENERATE PERLIN NOISE

	srand(static_cast<unsigned int>(time(NULL)));

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;

	std::string windowName = "Perlin Noise Generation";
	sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), (windowName + " " + GetEnvironmentBit()), sf::Style::Default, settings);
	window.setFramerateLimit(FPS);

	sf::Color backgroundColour = sf::Color(255, 0, 0);

	sf::Font ArialFont;
	ArialFont.loadFromFile("C:/Windows/Fonts/arial.ttf");

	sf::RenderTexture rt;
	rt.create(MapWidth, MapHeight);
	rt.clear(backgroundColour);

	sf::Event ev;
	sf::Clock clock;


	bool shouldRender = true;
	bool shouldSave = false;

	std::thread t1(processCommands, std::ref(RGBPerlinNoise), std::ref(shouldRender), std::ref(shouldSave));
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

					sf::Color colour = RGBPerlinNoise[row][column];

					testTile.setFillColor(colour);

					testTile.setPosition(sf::Vector2f((float)column, (float)row));
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