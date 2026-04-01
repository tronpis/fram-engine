#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

struct TileConfig {
    uint32_t tileID;
    std::string name;
    bool walkable;
    bool buildable;
    float fertility;
    float moistureRetention;
    uint32_t textureIndex;
};

class TileDatabase {
public:
    static TileDatabase& getInstance();
    
    void initialize();
    
    const TileConfig* getTile(uint32_t tileID) const;
    uint32_t getTileIDByName(const std::string& name) const;
    
    // Tiles de granja predefinidos
    uint32_t getDirtTile() const { return dirtTileID; }
    uint32_t getGrassTile() const { return grassTileID; }
    uint32_t getFarmlandTile() const { return farmlandTileID; }
    uint32_t getWateredFarmlandTile() const { return wateredFarmlandTileID; }
    uint32_t getPathTile() const { return pathTileID; }
    uint32_t getStoneTile() const { return stoneTileID; }
    uint32_t getWoodTile() const { return woodTileID; }
    
private:
    TileDatabase() = default;
    
    std::unordered_map<uint32_t, TileConfig> tiles;
    std::unordered_map<std::string, uint32_t> nameToID;
    
    uint32_t dirtTileID = 1;
    uint32_t grassTileID = 2;
    uint32_t farmlandTileID = 3;
    uint32_t wateredFarmlandTileID = 4;
    uint32_t pathTileID = 5;
    uint32_t stoneTileID = 6;
    uint32_t woodTileID = 7;
};

// Tilemap para granjas 2D
struct TilemapConfig {
    uint32_t width;
    uint32_t height;
    uint32_t tileSize;
    uint32_t layerCount;
};

enum class TileLayer {
    Ground = 0,
    Objects = 1,
    Crops = 2,
    Decorations = 3,
    Overlay = 4,
    LayerCount
};

class Tilemap {
public:
    Tilemap(const TilemapConfig& config);
    ~Tilemap();
    
    // Dimensiones
    uint32_t getWidth() const { return config.width; }
    uint32_t getHeight() const { return config.height; }
    uint32_t getTileSize() const { return config.tileSize; }
    
    // Acceso a tiles
    uint32_t getTile(uint32_t x, uint32_t y, TileLayer layer) const;
    void setTile(uint32_t x, uint32_t y, TileLayer layer, uint32_t tileID);
    
    // Acceso por coordenadas del mundo
    uint32_t getTileAtWorld(float worldX, float worldY, TileLayer layer) const;
    glm::vec2 getWorldPosition(uint32_t tileX, uint32_t tileY) const;
    
    // Conversión de coordenadas
    glm::ivec2 worldToTile(float worldX, float worldY) const;
    glm::vec2 tileToWorld(uint32_t tileX, uint32_t tileY) const;
    
    // Propiedades de tiles
    bool isWalkable(uint32_t x, uint32_t y) const;
    bool isBuildable(uint32_t x, uint32_t y) const;
    float getFertility(uint32_t x, uint32_t y) const;
    
    // Carga/Guardado
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
    
    // Datos crudos
    const std::vector<uint32_t>& getLayerData(TileLayer layer) const;
    
private:
    TilemapConfig config;
    std::vector<std::vector<uint32_t>> layers;
    
    TileDatabase& tileDB;
};

// Sistema de colisión basado en tiles
class TilemapCollision {
public:
    TilemapCollision(const Tilemap* tilemap);
    
    bool checkCollision(float x, float y, float width, float height) const;
    bool isPointWalkable(float x, float y) const;
    
    // Pathfinding helper
    bool canMoveTo(uint32_t tileX, uint32_t tileY) const;
    
private:
    const Tilemap* tilemap;
};

// Editor de tilemaps
class TilemapEditor {
public:
    struct Brush {
        uint32_t tileID;
        TileLayer layer;
        uint32_t size;
        bool continuous;
    };
    
    TilemapEditor(Tilemap* tilemap);
    
    void setBrush(const Brush& brush);
    void applyBrush(uint32_t tileX, uint32_t tileY);
    void floodFill(uint32_t tileX, uint32_t tileY, uint32_t targetID, uint32_t fillID);
    
    // Herramientas
    void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
    void drawRectangle(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, bool filled);
    void copySelection(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
    void pasteSelection(uint32_t destX, uint32_t destY);
    
private:
    Tilemap* tilemap;
    Brush currentBrush;
    std::vector<uint32_t> clipboard;
    uint32_t clipboardWidth = 0;
    uint32_t clipboardHeight = 0;
};

} // namespace FarmEngine
