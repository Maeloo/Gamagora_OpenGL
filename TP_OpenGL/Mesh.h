#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdint.h>

#include "GL/glew.h"   

#include <glm\glm\glm.hpp>
#include <glm\glm\vec3.hpp>
#include <glm\glm\mat4x4.hpp>
#include <glm\glm\gtx\transform.hpp>
#include <glm\glm\gtc\matrix_transform.hpp>

typedef glm::vec3 Vector3;
typedef glm::vec2 Vector2;

/////////////////////////////
// Edge
struct Edge {
	uint32_t _vertexIndex[2];
	uint32_t _faceIndex[2];

	bool operator==( const Edge e ) {
		return
			( ( e._vertexIndex[0] == this->_vertexIndex[0] ) && ( e._vertexIndex[1] == this->_vertexIndex[1] ) ) ||
			( ( e._vertexIndex[1] == this->_vertexIndex[0] ) && ( e._vertexIndex[0] == this->_vertexIndex[1] ) );
	}
};
/////////////////////////////

/////////////////////////////
// Face
struct Face {
	uint32_t _verticesCount;
	std::vector<uint32_t> _vertexIndices;
	std::vector<uint32_t> _uvIndices;
	std::vector<uint32_t> _normalIndices;
};

/////////////////////////////
// Triangle
struct Triangle {
	Vector3 p1;
	Vector3 p2;
	Vector3 p3;
};

/////////////////////////////
// Mesh
class Mesh {

public:
	Mesh ( );
	~Mesh ( );

	static Mesh loadOBJ ( const std::string &fileName, bool indexData );
	static Mesh loadOFF ( const std::string &fileName, bool calculateNormalVertex );
	static void saveOFF ( const std::string &fileName, const Mesh &mesh );

	Triangle getTriangle ( const Face &face );

	std::string _name;
	std::string _type;
	std::vector<Vector3> _vertices;
	std::vector<Vector3> _indexVertices;
	std::vector<Vector2> _uvs;
	std::vector<Vector2> _indexUvs;
	std::vector<Vector3> _normals;
	std::vector<Vector3> _indexNormals;
	std::vector<Face> _faces;
	std::vector<Edge> _edges;
	
	Vector3 _center;
	uint32_t
		_indexVertexCount,
		_vertexCount,
		_facesCount,
		_edgesCount;

	void rotate ( float angle, Vector3 normal ) {
		std::cout << "Rotating mesh...\n";

		for ( unsigned int i = 0; i < _vertexCount; i++ ) {
			glm::mat4 trans = glm::rotate ( glm::mat4 ( 1.0f ), angle, normal );
			glm::vec4 tmpPoint = glm::vec4 ( _vertices[i], 1.f ) * trans;
			
			_vertices[i] = Vector3 ( tmpPoint / tmpPoint.w );
		}
	}

	void translate ( Vector3 t ) {
		std::cout << "Translating mesh...\n";

		for ( unsigned int i = 0; i < _vertexCount; i++ ) {
			_vertices[i] += t;
		}
	}

	void scale ( Vector3 s ) {
		std::cout << "Scaling mesh...\n";

		for ( unsigned int i = 0; i < _vertexCount; i++ ) {
			_vertices[i] *= s;
		}
	}

	void indexData ( );

	static double calculateMax ( Mesh &mesh );
	static void centerNormalizeMesh ( Mesh &mesh, const double &max );
	static void removeFaces ( Mesh &mesh, int count );
	static void buildEdges ( Mesh & mesh );
};

inline std::ostream& operator<<( std::ostream& os, const Face& obj ) {
	os << obj._verticesCount << " ";
	for ( uint32_t i = 0; i < obj._verticesCount; ++i ) {
		os << obj._vertexIndices[i] << " ";
	}
	os << std::endl;
	return os;
}

inline std::ostream& operator<<( std::ostream& os, const Vector3& obj ) {
	os << obj.x << " " << obj.y << " " << obj.z;
	os << std::endl;
	return os;
}
