/*! \file */ 
#ifndef _OUTPUT_OBJECT_H
#define _OUTPUT_OBJECT_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "geom.h"
#include "coordinates.h"
#include "attribute_store.h"
#include "osm_store.h"

// Protobuf
#include "osmformat.pb.h"
#include "vector_tile.pb.h"

enum OutputGeometryType { POINT_, LINESTRING_, MULTILINESTRING_, POLYGON_ };

#define OSMID_TYPE_OFFSET	39
#define OSM_TYPE_BITS		2
#define OSMID_MASK 		((1ULL<<OSMID_TYPE_OFFSET)-1)
#define OSMID_SHAPE 	(0ULL<<OSMID_TYPE_OFFSET)
#define OSMID_NODE 		(1ULL<<OSMID_TYPE_OFFSET)
#define OSMID_WAY 		(2ULL<<OSMID_TYPE_OFFSET)
#define OSMID_RELATION 	(3ULL<<OSMID_TYPE_OFFSET)

//\brief Display the geometry type
std::ostream& operator<<(std::ostream& os, OutputGeometryType geomType);

/**
 * \brief OutputObject - any object (node, linestring, polygon) to be outputted to tiles

 * Possible future improvements to save memory:
 * - use a global dictionary for attribute key/values
*/
#pragma pack(push, 4)
class OutputObject {

protected:	
	OutputObject(OutputGeometryType type, uint_least8_t l, NodeID id, bool ascSort, AttributeStoreRef attributes, uint mz)
		: objectID(id), ascendingSort(ascSort), geomType(type), layer(l), z_order(0),
		  minZoom(mz), attributes(attributes)
	{ }


public:
	NodeID objectID 			: OSMID_TYPE_OFFSET + OSM_TYPE_BITS; // id of way (linestring/polygon) or node (point)
	bool ascendingSort			: 1;					// Sort ascending by z_order?
	uint_least8_t layer 		: 8;					// what layer is it in?
	OutputGeometryType geomType : 2;					// point, linestring, polygon
	unsigned minZoom 			: 4;
	float z_order;					// z_order: used for sorting features within layers

	AttributeStoreRef attributes;

	void setZOrder(const float z) {
		z_order = z;
	}

	void setMinZoom(unsigned z) {
		minZoom = z;
	}

	void setAttributeSet(AttributeStoreRef attributes) {
		this->attributes = attributes;
	}

	//\brief Write attribute key/value pairs (dictionary-encoded)
	void writeAttributes(std::vector<std::string> *keyList, 
		std::vector<vector_tile::Tile_Value> *valueList, vector_tile::Tile_Feature *featurePtr, char zoom) const;
	
	/**
	 * \brief Find a value in the value dictionary
	 * (we can't easily use find() because of the different value-type encoding - 
	 *	should be possible to improve this though)
	 */
	int findValue(std::vector<vector_tile::Tile_Value> *valueList, vector_tile::Tile_Value const &value) const;
};
#pragma pack(pop)

/**
 * \brief An OutputObject derived class that contains data originally from OsmMemTiles
*/
class OutputObjectOsmStorePoint : public OutputObject
{
public:
	OutputObjectOsmStorePoint(OutputGeometryType type, uint_least8_t l, NodeID id, bool ascSort, AttributeStoreRef attributes, uint minzoom)
		: OutputObject(type, l, id, ascSort, attributes, minzoom)
	{ 
		assert(type == POINT_);
	}
}; 

class OutputObjectOsmStoreLinestring : public OutputObject
{
public:
	OutputObjectOsmStoreLinestring(OutputGeometryType type, uint_least8_t l, NodeID id, bool ascSort, AttributeStoreRef attributes, uint minzoom)
		: OutputObject(type, l, id, ascSort, attributes, minzoom)
	{ 
		assert(type == LINESTRING_);
	}
};

class OutputObjectOsmStoreMultiLinestring : public OutputObject
{
public:
	OutputObjectOsmStoreMultiLinestring(OutputGeometryType type, uint_least8_t l, NodeID id, bool ascSort, AttributeStoreRef attributes, uint minzoom)
		: OutputObject(type, l, id, ascSort, attributes, minzoom)
	{ 
		assert(type == MULTILINESTRING_);
	}
};


class OutputObjectOsmStoreMultiPolygon : public OutputObject
{
public:
	OutputObjectOsmStoreMultiPolygon(OutputGeometryType type, uint_least8_t l, NodeID id, bool ascSort, AttributeStoreRef attributes, uint minzoom)
		: OutputObject(type, l, id, ascSort, attributes, minzoom)
	{ 
		assert(type == POLYGON_);
	}
};

class OutputObjectRef
{
	OutputObject *oo;

public:
	OutputObjectRef(OutputObject *oo = nullptr)
		: oo(oo)
	{ }
	OutputObjectRef(OutputObjectRef const &other) = default;
	OutputObjectRef(OutputObjectRef &&other) = default;

	OutputObjectRef &operator=(OutputObjectRef const &other) { oo = other.oo; return *this; }
    OutputObject& operator*() { return *oo; }
    OutputObject const& operator*() const { return *oo; }
    OutputObject *operator->() { return oo; }
    OutputObject const *operator->() const { return oo; }
	void reset() { oo = nullptr; }
};

/** \brief Assemble a linestring or polygon into a Boost geometry, and clip to bounding box
 * Returns a boost::variant -
 *	 POLYGON->MultiPolygon, CENTROID->Point, LINESTRING->MultiLinestring
 */
Geometry buildWayGeometry(OSMStore &osmStore, OutputObject const &oo, const TileBbox &bbox);

//\brief Build a node geometry
LatpLon buildNodeGeometry(OSMStore &osmStore, OutputObject const &oo, const TileBbox &bbox);

// Comparison functions

bool operator==(const OutputObjectRef x, const OutputObjectRef y);

/**
 * Do lexicographic comparison, with the order of: layer, geomType, attributes, and objectID.
 * Note that attributes is preferred to objectID.
 * It is to arrange objects with the identical attributes continuously.
 * Such objects will be merged into one object, to reduce the size of output.
 */
bool operator<(const OutputObjectRef x, const OutputObjectRef y);

namespace vector_tile {
	bool operator==(const vector_tile::Tile_Value &x, const vector_tile::Tile_Value &y);
	bool operator<(const vector_tile::Tile_Value &x, const vector_tile::Tile_Value &y);
}

namespace std {
	/// Hashing function so we can use an unordered_set
	template<>
	struct hash<OutputObjectRef> {
		size_t operator()(const OutputObjectRef &oo) const {
			return std::hash<uint_least8_t>()(oo->layer) ^ std::hash<NodeID>()(oo->objectID);
		}
	};
}

#endif //_OUTPUT_OBJECT_H
