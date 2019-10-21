/**
 * @file WAssimpImporter.hpp
 * @brief An asset importer that uses the Assimp library
 *
 * @author Hasan Al-Jawaheri (hbj)
 * @bug No known bugs.
 */

#pragma once

#include "Wasabi/Core/WCore.hpp"

/**
 * @ingroup engineclass
 * 
 * A utility class that helps loading assets from different file formats.
 */
class WAssimpImporter {
public:
    WAssimpImporter(class Wasabi* const app);
	~WAssimpImporter();

    WError LoadSingleObject(std::string filename, class WObject*& object);

    WError LoadScene(
        std::string filename,
        std::vector<class WObject*>& objects,
        std::vector<class WImage*>& textures,
        std::vector<class WGeometry*>& geometries,
		std::vector<class WLight*> lights,
		std::vector<class WCamera*> cameras
    );

private:
    class Wasabi* m_app;
};
