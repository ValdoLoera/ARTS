#include "map.h"

Map::Map()
{
}

Map::~Map()
{
}

bool Map::LoadFile(std::string filename)
{
	// Cargamos el archivo de mapa
	pugi::xml_document doc;
	if (!doc.load_file(filename.c_str()))
	{
		std::cerr << "Map::LoadFile: Error al cargar el fichero xml " << filename << std::endl;
		return false;
	}

	// Cogemos el nodo principal
	pugi::xml_node root_node;
	if (!(root_node = doc.child("map")))
	{
		std::cerr << "Map::LoadFile: Error al leer el fichero xml en elemento map" << std::endl;
		return false;
	}

	// Tamaño del mapa
	this->width = root_node.attribute("width").as_uint();
	this->height = root_node.attribute("height").as_uint();

	// Tamaño de los tiles
	this->tile_width = root_node.attribute("tilewidth").as_uint();
	this->tile_height = root_node.attribute("tileheight").as_uint();

	// Recorremos todos los nodos del mapa
	for (pugi::xml_node node = root_node.first_child(); node; node = node.next_sibling())
	{
		// Obtenemos el nombre del nodo
		std::string name = node.name();

		// Si es el nodo de propiedades
		if (name == "properties")
		{
			// Recorremos las propiedades
			for (pugi::xml_node property = node.first_child(); property; property = property.next_sibling())
			{
				// Extraemos el nombre
				std::string name = property.attribute("name").value();
				if (name == "name")
					this->name = property.attribute("value").value();
			}
		}

		// Si es un nodo de tileset
		else if (name == "tileset")
		{
			// Creamos un objeto Tileset
			Tileset tileset;

			// Nombre del tileset
			tileset.name = node.attribute("name").value();

			// Número del primer tile
			tileset.firstgid = node.attribute("firstgid").as_uint();

			// Tamaño de los tiles
			tileset.tile_width = node.attribute("tilewidth").as_uint();;
			tileset.tile_height = node.attribute("tileheight").as_uint();;

			// Obtenemos el nodo image
			pugi::xml_node image = node.child("image");

			// Ruta de la imagen del tileset
			tileset.src_image =  image.attribute("source").value();

			// Tamaño de la imagen
			tileset.width = image.attribute("width").as_uint();
			tileset.height = image.attribute("height").as_uint();

			// Añadimos el tileset a la lista de tilesets
			this->tilesets.push_back(tileset);
		}
		
		// Si es un nodo de layer
		else if (name == "layer")
		{
			// Creamos un objeto Layer
			Layer layer;

			// Nombre da la capa
			layer.name = node.attribute("name").value();

			// Tamaño de la capa
			layer.width = node.attribute("width").as_uint();
			layer.height = node.attribute("height").as_uint();

			// Obtenemos el nodo data
			pugi::xml_node data = node.child("data");

			// Tipo de compresión
			const std::string compression = data.attribute("compression").value();
			
			// Descomprimimos y decodifamos la capa
			int len = strlen((const char*)data.child_value()) + 1;
			unsigned char *charData = new unsigned char[len + 1];
			const char *charStart = (const char*) data.child_value();
			unsigned char *charIndex = charData;

			while (*charStart)
			{
				if (*charStart != ' ' && *charStart != '\t' &&
				        *charStart != '\n')
				{
					*charIndex = *charStart;
					charIndex++;
				}
				charStart++;
			}
			*charIndex = '\0';

			int binLen;
			unsigned char *binData = php3_base64_decode(charData, strlen((char*)charData), &binLen);

			delete[] charData;

			if (binData)
			{
				if (compression == "gzip" || compression == "zlib")
				{
					// Inflate the gzipped layer data
					unsigned char *inflated;
					unsigned int inflatedSize = inflateMemory(binData, binLen, inflated);

					//free(binData);
					binData = inflated;
					binLen = inflatedSize;

					if (!inflated)
					{
						std::cerr << "Error: Could not decompress layer!" << std::endl;
						return false;
					}
				}
				for (int i = 0; i < binLen - 3; i += 4)
				{
					const int gid = binData[i] | binData[i + 1] << 8 | binData[i + 2] << 16 | binData[i + 3] << 24;
					layer.data.push_back(gid);
				}
				//free(binData);
			}
			
			// Añadimos la capa a la lista de capas
			this->layers.push_back(layer);
		}
	}
}