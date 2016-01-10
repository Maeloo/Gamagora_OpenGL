#include "Mesh.h"


Mesh::Mesh ( ) {
}


Mesh::~Mesh ( ) {
}

// Charge un fichier OBJ
Mesh Mesh::loadOBJ ( const std::string &fileName, bool calculateNormalVertex ) {
	Mesh mesh = Mesh ( );

	FILE * file = fopen ( fileName.c_str ( ), "r" );

	if ( file == NULL ) {
		printf ( "Impossible to open the file !\n" );
		return mesh;
	}

	mesh._name = fileName;

	mesh._type = "OBJ";

	mesh._vertices	= std::vector<Vector3> ( );
	mesh._uvs		= std::vector<Vector2> ( );
	mesh._normalsF	= std::vector<Vector3> ( );
	mesh._faces		= std::vector<Face> ( );

	Vector3 center;

	bool addUVs		= false;
	bool addNormal	= false;

	while ( 1 ) {
		char lineHeader[128];

		// read the first word of the line
		int res = fscanf ( file, "%s", lineHeader );
		if ( res == EOF )
			break; // EOF = End Of File. Quit the loop.

		if ( strcmp ( lineHeader, "v" ) == 0 ) {
			Vector3 vertex;
			
			fscanf ( file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			
			mesh._vertices.push_back ( vertex );

			center += vertex;
		}
		else if ( strcmp ( lineHeader, "vt" ) == 0 ) {
			glm::vec2 uv;
			
			fscanf ( file, "%f %f\n", &uv.x, &uv.y );
			
			mesh._uvs.push_back ( uv );

			addUVs = true;
		}
		else if ( strcmp ( lineHeader, "vn" ) == 0 ) {
			glm::vec3 normal;
			
			fscanf ( file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			
			mesh._normalsF.push_back ( normal );

			addNormal = true;
		}
		else if ( strcmp ( lineHeader, "f" ) == 0 && addNormal && addUVs ) {
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			fscanf ( file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );

			Face face;
			face._verticesCount = 3;
			face._vertexIndices.push_back ( vertexIndex[0] - 1);
			face._vertexIndices.push_back ( vertexIndex[1] - 1 );
			face._vertexIndices.push_back ( vertexIndex[2] - 1 );
			face._uvIndices.push_back ( uvIndex[0] - 1 );
			face._uvIndices.push_back ( uvIndex[1] - 1 );
			face._uvIndices.push_back ( uvIndex[2] - 1 );
			face._normalIndices.push_back ( normalIndex[0] - 1 );
			face._normalIndices.push_back ( normalIndex[1] - 1 );
			face._normalIndices.push_back ( normalIndex[2] - 1 );

			mesh._faces.push_back ( face );
		}
		else if ( strcmp ( lineHeader, "f" ) == 0 && addUVs ) {
			unsigned int vertexIndex[3], uvIndex[3];

			fscanf ( file, "%d/%d %d/%d %d/%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2] );

			Face face;
			face._verticesCount = 3;
			face._vertexIndices.push_back ( vertexIndex[0] - 1 );
			face._vertexIndices.push_back ( vertexIndex[1] - 1 );
			face._vertexIndices.push_back ( vertexIndex[2] - 1 );
			face._uvIndices.push_back ( uvIndex[0] - 1 );
			face._uvIndices.push_back ( uvIndex[1] - 1 );
			face._uvIndices.push_back ( uvIndex[2] - 1 );

			mesh._faces.push_back ( face );
		}
		else if ( strcmp ( lineHeader, "f" ) == 0 && addNormal ) {
			unsigned int vertexIndex[3], normalIndex[3];

			fscanf ( file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2] );

			Face face;
			face._verticesCount = 3;
			face._vertexIndices.push_back ( vertexIndex[0] - 1 );
			face._vertexIndices.push_back ( vertexIndex[1] - 1 );
			face._vertexIndices.push_back ( vertexIndex[2] - 1 );
			face._normalIndices.push_back ( normalIndex[0] - 1 );
			face._normalIndices.push_back ( normalIndex[1] - 1 );
			face._normalIndices.push_back ( normalIndex[2] - 1 );

			mesh._faces.push_back ( face );
		}
	}

	mesh._verticesCount = mesh._vertices.size ( );
	mesh._facesCount = mesh._faces.size ( );

	// Calcule du centre de gravité
	center /= mesh._verticesCount;
	mesh._center = center;

	double max = calculateMax ( mesh );

	centerNormalizeMesh ( mesh, max );

	if ( !addNormal ) {
		// Calcule des normales par face
		std::cout << "Calculate face normals...\n";
		mesh._normalsF = std::vector<Vector3> ( mesh._facesCount );
		for ( uint32_t k = 0; k < mesh._facesCount; ++k ) {
			Face face = mesh._faces[k];

			Vector3 normal = glm::normalize ( glm::cross ( mesh._vertices[face._vertexIndices[1]] - mesh._vertices[face._vertexIndices[0]], mesh._vertices[face._vertexIndices[2]] - mesh._vertices[face._vertexIndices[0]] ) );
			mesh._normalsF[k] = normal;
		}
	}	

	if ( calculateNormalVertex ) {
		// Calcule des normales par vertex
		std::cout << "Calculate vertex normals...\n";
		mesh._normalsV = std::vector<Vector3> ( mesh._verticesCount );
		for ( uint32_t idx = 0; idx < mesh._verticesCount; ++idx ) {
			Vector3 normal = { .0f, .0f, .0f };

			for ( uint32_t m = 0; m < mesh._facesCount; ++m ) {
				Face face = mesh._faces[m];

				for ( uint32_t n = 0; n < face._verticesCount; ++n ) {
					if ( face._vertexIndices[n] == idx ) {
						normal += mesh._normalsF[m];
					}
				}
			}

			mesh._normalsV[idx] = glm::normalize ( normal );
		}
	}

	//buildEdges ( mesh );

	return mesh;
}


// Récupere un triangle à partir d'une face
Triangle Mesh::getTriangle ( const Face &face ) {
	Triangle res = {
		_vertices[face._vertexIndices[0]],
		_vertices[face._vertexIndices[1]],
		_vertices[face._vertexIndices[2]]
	};

	return res;
}

// Sauvegarde un fichier .OFF
void Mesh::saveOFF ( const std::string &fileName, const Mesh &mesh ) {
	std::ofstream file;
	file.open ( fileName );
	file << "OFF" << std::endl;
	file << mesh._verticesCount << " " << mesh._facesCount << " " << mesh._edgesCount << std::endl;

	for ( uint32_t i = 0; i < mesh._verticesCount; ++i ) {
		file << mesh._vertices[i];
	}

	for ( uint32_t i = 0; i < mesh._facesCount; ++i ) {
		file << mesh._faces[i];
	}

	file.close ( );
}

// Charge un fichier .OFF
Mesh Mesh::loadOFF ( const std::string &fileName, bool calculateNormalVertex ) {
	std::cout << "Loading file...\n";

	// Ouverture du fichier dans un stream
	std::ifstream file ( fileName );

	Mesh mesh;

	mesh._name = fileName;

	// Type
	file >> mesh._type;

	// Infos
	file >> mesh._verticesCount >> mesh._facesCount >> mesh._edgesCount;

	Vector3 center;

	// Read vertex
	mesh._vertices = std::vector<Vector3> ( mesh._verticesCount );
	for ( uint32_t i = 0; i < mesh._verticesCount; ++i ) {
		Vector3 v;
		file >> v.x >> v.y >> v.z;
		center += v;
		mesh._vertices[i] = v;
	}

	// Calcule du centre de gravité
	center /= mesh._verticesCount;
	mesh._center = center;

	// Read faces
	mesh._faces = std::vector<Face> ( mesh._facesCount );
	for ( uint32_t j = 0; j < mesh._facesCount; ++j ) {
		Face f;
		file >> f._verticesCount;
		f._vertexIndices = std::vector<uint32_t> ( f._verticesCount );

		for ( uint32_t k = 0; k < f._verticesCount; ++k ) {
			file >> f._vertexIndices[k];
		}

		mesh._faces[j] = f;
	}

	double max = calculateMax ( mesh );

	centerNormalizeMesh ( mesh, max );

	// Calcule des normales par face
	std::cout << "Calculate face normals...\n";
	mesh._normalsF = std::vector<Vector3> ( mesh._facesCount );
	for ( uint32_t k = 0; k < mesh._facesCount; ++k ) {
		Face face = mesh._faces[k];
		
		Vector3 normal = glm::normalize ( glm::cross ( mesh._vertices[face._vertexIndices[1]] - mesh._vertices[face._vertexIndices[0]], mesh._vertices[face._vertexIndices[2]] - mesh._vertices[face._vertexIndices[0]] ) );
		mesh._normalsF[k] = normal;
	}

	if ( calculateNormalVertex ) {
		// Calcule des normales par vertex
		std::cout << "Calculate vertex normals...\n";
		mesh._normalsV = std::vector<Vector3> ( mesh._verticesCount );
		for ( uint32_t idx = 0; idx < mesh._verticesCount; ++idx ) {
			Vector3 normal = { .0f, .0f, .0f };

			for ( uint32_t m = 0; m < mesh._facesCount; ++m ) {
				Face face = mesh._faces[m];

				for ( uint32_t n = 0; n < face._verticesCount; ++n ) {
					if ( face._vertexIndices[n] == idx ) {
						normal += mesh._normalsF[m];
					}
				}
			}

			mesh._normalsV[idx] = glm::normalize ( normal );
		}
	}
	
	//buildEdges ( mesh );

	return mesh;
}

// Centre et normalise le mesh
void Mesh::centerNormalizeMesh ( Mesh &mesh, const double &max ) {
	std::cout << "Center & Normalize Mesh...\n";
	float p = 1.0f / mesh._verticesCount;
	double inv = 1 / max;

	for ( uint32_t i = 0; i < mesh._verticesCount; ++i ) {
		mesh._vertices[i] -= mesh._center;
		mesh._vertices[i] *= inv;
	}
}

// Calcule le max
double Mesh::calculateMax ( Mesh &mesh ) {
	std::cout << "Calculate max...\n";
	float p = 1.0f / mesh._verticesCount;
	double max = 0;

	for ( uint32_t i = 0; i < mesh._verticesCount; ++i ) {
		// Calcule coordonnées max absolue
		Vector3 vertex = mesh._vertices[i];
		double abs;
		abs = fabs ( vertex.x - mesh._center.x );
		max = max > abs ? max : abs;
		abs = fabs ( vertex.y - mesh._center.y );
		max = max > abs ? max : abs;
		abs = fabs ( vertex.z - mesh._center.z );
		max = max > abs ? max : abs;
	}

	return max;
}
