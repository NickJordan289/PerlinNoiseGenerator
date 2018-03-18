#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

#if _WIN32 || _WIN64
	#if _WIN64
		//#define ENVIRONMENT64 // no need to define because we can just use #if _WIN32
		std::string ENVIRONMENTBIT = "64-bit";
	#else
		#if _WIN32
			//#define ENVIRONMENT32 // no need to define because we can just use #if _WIN32
			std::string ENVIRONMENTBIT = "32-bit";
		#endif
	#endif
#else
		//#define ENVIRONMENT00 // no need to define because we can just use #if _WIN32
		std::string ENVIRONMENTBIT = "00-bit";
#endif

typedef long long int64; 
typedef unsigned long long uint64;

std::string GetEnvironmentBit() {
	return ENVIRONMENTBIT;
}

template <typename T>
T rNum(T Min, T Max) { return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min; }
template <typename T>
T rNum(T Max) { return T((float(rand()) / float(RAND_MAX)) * (Max - 0)) + 0; }

template <typename T>
inline T lerp(T v0, T v1, T t) {
    return (1-t)*v0 + t*v1;
}

template <typename T>
inline T Interpolate(T x0, T x1, T alpha) {
	return x0 * (1 - alpha) + alpha * x1;
}

double map(double n, double start1, double stop1, double start2, double stop2) {
	return ((n - start1) / (stop1 - start1))*(stop2 - start2) + start2;
};

float dist(sf::Vector2f a, sf::Vector2f b) {
	return std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2));
}

double RandomDouble(double a, double b) {
	double random = ((double)rand()) / (double)RAND_MAX;
	double diff = b - a;
	double r = random * diff;
	return a + r;
}

float Length(sf::Vector2f V) {
	return std::sqrt(V.x * V.x + V.y * V.y);
}

sf::Vector2f Normalize(sf::Vector2f V) {
	sf::Vector2f normalizedVector;
	float len = Length(V);
	if (len != 0) {
		normalizedVector = sf::Vector2f(V.x/len, V.y/len);
	}
	return normalizedVector;
}

template <typename T>
T constrain(T n, float low, float high) {
	return std::max(std::min(n, high), low);
};

float TruncateRGB(float n) {
	return std::max(std::min(n, 255.f), 0.f);
}

sf::Color FloatToColour(float value) {
	return sf::Color(sf::Uint8(value*255.f), sf::Uint8(value*255.f), sf::Uint8(value*255.f));
}

int RGBToDec(sf::Color c)
{
	return (c.r << 16) | (c.g << 8) | c.b;
}

sf::Color RGBFromDec(int d)
{
	sf::Color temp;
	temp.r = (d >> 16) & 255;
	temp.g = (d >> 8) & 255;
	temp.b = d & 255;
	return temp;
}

template <typename T>
void delete_pointed_to(T* const ptr) {
	delete ptr;
}

template <typename T>
void free_pointer_vector_memory(std::vector<T*> ptr_vector) {
	std::for_each(ptr_vector.begin(), ptr_vector.end(), delete_pointed_to<T>);
}
