/*
 * tile_expiry_list.h
 *
 *  Created on:  2021-11-05
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef INCLUDE_TILE_EXPIRY_LIST_H_
#define INCLUDE_TILE_EXPIRY_LIST_H_

#include <vector>
#include "coordinates.h"

class TileExpiryList {
	unsigned int baseZoom;
	bool enabled;
	std::vector<TileCoordinates> tileCoords;

	void prepareListForQuery();

public:
	TileExpiryList() = delete;

	TileExpiryList(unsigned int zoom, bool enabled);

	TileExpiryList(unsigned int zoom, bool enabled, std::vector<TileCoordinates>&& tiles);

	bool isEnabled() const;

	/**
	 * Read tile expiry list from file name.
	 *
	 * @throws runtime_error if zoom level is different from baseZoom
	 */
	void readListFromFile(const char* fileName);

	/**
	 * Check if provided tile coordinate is in the list of expired tiles.
	 */
	bool contains(const TileCoordinates& coord) const;

	/**
	 * Clone this instance and convert all tile coordinates to the destination zoom level.
	 *
	 * If destZoom is equal to baseZoom, this instance is returned and the old instance
	 * is
	 *
	 * @param destZoom destination zoom level. Zoom levels greater than baseZoom result in
	 * undefined behaviour.
	 */
	std::vector<TileCoordinates> clone_at_zoom(unsigned int destZoom) const;
};



#endif /* INCLUDE_TILE_EXPIRY_LIST_H_ */
