#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class CropType {
    Wheat,
    Corn,
    Tomato,
    Carrot,
    Potato,
    Pumpkin,
    Sunflower,
    Strawberry,
    Blueberry
};

enum class CropGrowthStage {
    Seed,
    Sprout,
    Growing,
    Mature,
    Ready
};

struct CropConfig {
    CropType type;
    std::string name;
    float growthTime;  // Tiempo base en segundos
    int32_t stagesCount;
    float waterNeed;   // 0.0 - 1.0
    float temperatureMin;
    float temperatureMax;
    float fertilityNeed;
    uint32_t seedYield;
    uint32_t harvestYield;
};

class Crop {
public:
    Crop(CropType type, const glm::vec3& position);
    
    void update(float deltaTime);
    
    // Estado
    CropGrowthStage getGrowthStage() const { return growthStage; }
    float getGrowthProgress() const { return growthProgress; }
    bool isReadyToHarvest() const { return growthStage == CropGrowthStage::Ready; }
    bool isDead() const { return dead; }
    
    // Acciones
    bool water();
    bool fertilize();
    bool harvest();
    
    // Factores ambientales
    void setWaterLevel(float level) { waterLevel = level; }
    void setFertility(float fertility) { soilFertility = fertility; }
    void setTemperature(float temp) { temperature = temp; }
    
    // Datos
    const CropConfig& getConfig() const;
    uint32_t getYield() const { return currentYield; }
    
private:
    void updateGrowth(float deltaTime);
    void checkConditions();
    void advanceStage();
    
    CropType cropType;
    glm::vec3 position;
    
    CropGrowthStage growthStage = CropGrowthStage::Seed;
    float growthProgress = 0.0f;
    float waterLevel = 0.5f;
    float soilFertility = 0.5f;
    float temperature = 20.0f;
    
    uint32_t currentYield = 0;
    bool dead = false;
    bool watered = false;
};

class CropManager {
public:
    static CropManager& getInstance();
    
    void initialize();
    void cleanup();
    
    // Creación
    Crop* plantCrop(CropType type, const glm::vec3& position, uint32_t fieldID);
    
    // Gestión por campo
    std::vector<Crop*> getCropsInField(uint32_t fieldID);
    void removeCropsFromField(uint32_t fieldID);
    
    // Actualización
    void update(float deltaTime);
    void updateRegion(const glm::vec3& center, float radius);
    
    // Consultas
    Crop* getCropAtPosition(const glm::vec3& position);
    std::vector<Crop*> getCropsByType(CropType type);
    std::vector<Crop*> getReadyCrops();
    
    // Estadísticas
    size_t getTotalCropCount() const { return crops.size(); }
    size_t getReadyCropCount() const;
    
    // Configuración
    const CropConfig* getCropConfig(CropType type) const;
    
private:
    CropManager() = default;
    ~CropManager();
    
    std::unordered_map<uint32_t, std::unique_ptr<Crop>> crops;
    std::unordered_map<CropType, CropConfig> cropConfigs;
    std::unordered_map<uint32_t, std::vector<uint32_t>> fieldCrops; // fieldID -> crop IDs
    
    uint32_t nextCropID = 1;
};

// Sistema de rotación de cultivos
class CropRotation {
public:
    struct RotationPlan {
        std::vector<CropType> sequence;
        int32_t seasonsPerCrop;
        int32_t restSeasons;
    };
    
    static CropRotation& getInstance();
    
    void setRotationPlan(uint32_t fieldID, const RotationPlan& plan);
    CropType getNextCropForField(uint32_t fieldID);
    
    // Beneficios de rotación
    float getFertilityBonus(uint32_t fieldID) const;
    float getPestResistanceBonus(uint32_t fieldID) const;
    
private:
    CropRotation() = default;
    
    std::unordered_map<uint32_t, RotationPlan> fieldPlans;
    std::unordered_map<uint32_t, int32_t> fieldHistory; // Última posición en secuencia
};

} // namespace FarmEngine
