#pragma once

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class SoilType {
    Sandy,
    Loamy,
    Clay,
    Silty,
    Peaty,
    Chalky
};

enum class SoilCondition {
    Poor,
    Average,
    Good,
    Excellent
};

struct SoilData {
    SoilType type = SoilType::Loamy;
    float moisture = 0.5f;      // 0.0 - 1.0
    float fertility = 0.5f;     // 0.0 - 1.0
    float pH = 7.0f;            // 3.0 - 9.0
    float nitrogen = 0.5f;      // NPK values
    float phosphorus = 0.5f;
    float potassium = 0.5f;
    float organicMatter = 0.5f;
    float temperature = 20.0f;  // Celsius
    uint8_t tillageState = 0;   // 0 = untilled, 1 = tilled
};

class Soil {
public:
    Soil(const glm::ivec3& position);
    
    void update(float deltaTime);
    
    // Propiedades básicas
    SoilType getType() const { return data.type; }
    float getMoisture() const { return data.moisture; }
    float getFertility() const { return data.fertility; }
    float getpH() const { return data.pH; }
    bool isTilled() const { return data.tillageState > 0; }
    
    // Acciones
    bool till();
    bool water(float amount);
    bool fertilize(float nitrogen, float phosphorus, float potassium);
    bool addOrganicMatter(float amount);
    bool adjustPH(float amount);  // Positive = more alkaline, negative = more acidic
    
    // Estado
    SoilCondition getCondition() const;
    float getQualityScore() const;  // 0.0 - 1.0
    
    // Factores ambientales
    void setTemperature(float temp) { data.temperature = temp; }
    void absorbRainfall(float amount);
    void evaporate(float rate);
    
    // Datos completos
    const SoilData& getData() const { return data; }
    void setData(const SoilData& newData) { data = newData; }
    
private:
    void updateMoisture(float deltaTime);
    void updateNutrients(float deltaTime);
    void updateTemperature(float deltaTime);
    void calculateCondition();
    
    glm::ivec3 position;
    SoilData data;
    SoilCondition condition = SoilCondition::Average;
};

class SoilManager {
public:
    static SoilManager& getInstance();
    
    void initialize();
    void cleanup();
    
    // Acceso a suelo
    Soil* getSoil(const glm::ivec3& position);
    Soil* getSoilAtWorld(const glm::vec3& worldPosition);
    
    // Operaciones en área
    void tillArea(const glm::vec3& center, float radius);
    void waterArea(const glm::vec3& center, float radius, float amount);
    void fertilizeArea(const glm::vec3& center, float radius, 
                      float n, float p, float k);
    
    // Actualización
    void update(float deltaTime);
    void updateRegion(const glm::vec3& center, float radius, float deltaTime);
    
    // Análisis
    float getAverageFertility(const glm::vec3& center, float radius) const;
    float getAverageMoisture(const glm::vec3& center, float radius) const;
    SoilCondition getDominantCondition(const glm::vec3& center, float radius) const;
    
    // Mapa de suelo (para rendering/analysis)
    std::vector<float> getMoistureMap(int32_t width, int32_t depth, 
                                      const glm::vec3& origin, float scale) const;
    std::vector<float> getFertilityMap(int32_t width, int32_t depth,
                                       const glm::vec3& origin, float scale) const;
    
private:
    SoilManager() = default;
    ~SoilManager();
    
    glm::ivec3 worldToGrid(const glm::vec3& worldPos) const;
    glm::vec3 gridToWorld(const glm::ivec3& gridPos) const;
    
    std::unordered_map<int64_t, std::unique_ptr<Soil>> soilGrid;
    
    // Configuración
    float gridScale = 1.0f;  // Tamaño de cada celda de suelo
};

// Sistema de erosión y degradación
class SoilErosion {
public:
    static SoilErosion& getInstance();
    
    void update(float deltaTime);
    
    // Causas de erosión
    void applyWaterErosion(const glm::vec3& start, const glm::vec3& end, float intensity);
    void applyWindErosion(const glm::vec3& direction, float intensity);
    void applyTillageErosion(const glm::vec3& position, float intensity);
    
    // Prevención
    void plantCoverCrop(const glm::vec3& center, float radius);
    void buildTerrace(const glm::vec3& start, const glm::vec3& end);
    void addMulch(const glm::vec3& center, float radius, float thickness);
    
private:
    SoilErosion() = default;
    
    void redistributeSoil(const glm::ivec3& from, const glm::ivec3& to, float amount);
};

// Compostaje y mejora de suelo
class CompostSystem {
public:
    struct CompostPile {
        std::vector<uint32_t> ingredients;
        float carbon = 0.0f;
        float nitrogen = 0.0f;
        float moisture = 0.0f;
        float temperature = 20.0f;
        float decompositionProgress = 0.0f;
        bool ready = false;
    };
    
    static CompostSystem& getInstance();
    
    uint32_t createCompostPile(const glm::vec3& position);
    void addIngredient(uint32_t pileID, uint32_t ingredientID, float amount);
    void turnCompost(uint32_t pileID);
    void waterCompost(uint32_t pileID, float amount);
    
    uint32_t harvestCompost(uint32_t pileID);
    
    void update(float deltaTime);
    
private:
    CompostSystem() = default;
    
    void updateDecomposition(CompostPile& pile, float deltaTime);
    float calculateIdealCNRatio() const;
    
    std::unordered_map<uint32_t, CompostPile> compostPiles;
    uint32_t nextPileID = 1;
};

} // namespace FarmEngine
