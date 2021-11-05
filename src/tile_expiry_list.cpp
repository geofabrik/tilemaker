/*
 * tile_expiry_list.cpp
 *
 *  Created on:  2021-11-05
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "tile_expiry_list.h"
#include <algorithm>
#include <fstream>
#include <boost/algorithm/string/split.hpp>

void TileExpiryList::prepareListForQuery() {
	if (enabled) {
		struct TileCoordinatesCompare comp;
		std::sort(tileCoords.begin(), tileCoords.end(), comp);
		std::unique(tileCoords.begin(), tileCoords.end());
	}
}

TileExpiryList::TileExpiryList(unsigned int zoom, bool enabled) :
		baseZoom(zoom),
		enabled(enabled),
		tileCoords() {}

TileExpiryList::TileExpiryList(unsigned int zoom, bool enabled, std::vector<TileCoordinates>&& tiles) :
		baseZoom(zoom),
		enabled(enabled),
		tileCoords(std::move(tiles)) {}

bool TileExpiryList::isEnabled() const {
	return enabled;
}

void TileExpiryList::readListFromFile(const char* fileName) {
	if (!enabled) {
		return;
	}
	std::ifstream expiryfile {fileName};
	std::string line;
	while (std::getline(expiryfile, line)) {
		std::vector<std::string> elements;
		// splitting the string
		boost::split(elements, line, boost::is_any_of("/"));

		int zoom = std::stoi(elements.at(0));
		if (zoom != baseZoom) {
			throw new std::runtime_error{"Tile expiry list contains entries of a zoom level other than the base zoom level."};
		}
		int x = std::stoi(elements.at(1));
		int y = std::stoi(elements.at(2));
		if (x < 0 || y < 0 || x >= (2 << baseZoom) || y >= (2 << baseZoom)) {
			throw new std::runtime_error{"Invalid tile to be expired: " + line};
		}
		tileCoords.emplace_back(x, y);
	}
	prepareListForQuery();
	expiryfile.close();
}

bool TileExpiryList::contains(const TileCoordinates& coord) const {
	struct TileCoordinatesCompare comp;
	return !enabled || std::binary_search(tileCoords.begin(), tileCoords.end(), coord, comp);
}

std::vector<TileCoordinates> TileExpiryList::clone_at_zoom(unsigned int destZoom) const {
	if (!enabled) {
		return std::vector<TileCoordinates>();
	}
	if (destZoom == baseZoom) {
		return std::move(tileCoords);
	}
	std::vector<TileCoordinates> newTiles {tileCoords};
	for (auto it = newTiles.begin(); it != newTiles.end(); ++it) {
		it->x = it->x >> (baseZoom - destZoom);
		it->y = it->y >> (baseZoom - destZoom);
	}
	struct TileCoordinatesCompare comp;
	std::sort(newTiles.begin(), newTiles.end(), comp);
	std::unique(newTiles.begin(), newTiles.end());
	return newTiles;
}
