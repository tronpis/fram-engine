#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class Season {
    Spring,
    Summer,
    Autumn,
    Winter
};

enum class WeatherType {
    Clear,
    Cloudy,
    Rainy,
    Stormy,
    Snowy,
    Foggy,
    Windy
};

struct WeatherConfig {
    float baseTemperature;      // Temperatura base de la estación
    float temperatureVariation; // Variación diaria
    float rainProbability;      // 0.0 - 1.0
    float windSpeed;            // m/s
    float humidity;             // 0.0 - 1.0
    int32_t dayLength;          // Horas de luz
};

class Weather {
public:
    Weather();
    
    void update(float deltaTime);
    
    // Estado actual
    WeatherType getType() const { return currentWeather; }
    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    float getWindSpeed() const { return windSpeed; }
    glm::vec3 getWindDirection() const { return windDirection; }
    float getRainIntensity() const { return rainIntensity; }
    float getCloudCover() const { return cloudCover; }
    
    // Predicción (próximas horas)
    WeatherType predictWeather(int32_t hoursAhead) const;
    float predictTemperature(int32_t hoursAhead) const;
    
    // Efectos en el juego
    bool isRaining() const { return currentWeather == WeatherType::Rainy || 
                                   currentWeather == WeatherType::Stormy; }
    bool isSnowing() const { return currentWeather == WeatherType::Snowy; }
    bool isStormy() const { return currentWeather == WeatherType::Stormy; }
    float getLightModifier() const;  // Modificador de luz ambiental
    
private:
    void updateTemperature(float deltaTime);
    void updateWeatherType(float deltaTime);
    void generateDailyForecast();
    
    WeatherType currentWeather = WeatherType::Clear;
    float temperature = 20.0f;
    float humidity = 0.5f;
    float windSpeed = 5.0f;
    glm::vec3 windDirection{1.0f, 0.0f, 0.0f};
    float rainIntensity = 0.0f;
    float cloudCover = 0.2f;
    
    float timeOfDay = 12.0f;  // Hora actual (0-24)
    int32_t currentDay = 1;
    
    std::vector<WeatherType> forecast;
};

class SeasonSystem {
public:
    static SeasonSystem& getInstance();
    
    void initialize(int32_t daysPerSeason = 30);
    void update(float deltaTime);
    
    // Estado
    Season getCurrentSeason() const { return currentSeason; }
    int32_t getCurrentDay() const { return currentDay; }
    int32_t getDaysPerSeason() const { return daysPerSeason; }
    float getSeasonProgress() const;  // 0.0 - 1.0
    
    // Configuración de estación
    const WeatherConfig& getSeasonConfig() const;
    
    // Efectos de estación
    float getGrowthModifier() const;  // Multiplicador de crecimiento de cultivos
    float getWaterEvaporationRate() const;
    int32_t getDayLengthHours() const;
    
    // Transiciones
    bool isSeasonTransition() const;  // Últimos días de la estación
    int32_t getDaysUntilTransition() const;
    
    // Forzar cambio (para debugging/eventos)
    void advanceSeason();
    void setSeason(Season season);
    
private:
    SeasonSystem() = default;
    
    void updateSeasonEffects();
    WeatherConfig calculateSeasonConfig() const;
    
    Season currentSeason = Season::Spring;
    int32_t currentDay = 1;
    int32_t daysPerSeason = 30;
    
    WeatherConfig seasonConfig;
    float growthModifier = 1.0f;
    float evaporationRate = 1.0f;
};

// Sistema climático global
class ClimateSystem {
public:
    static ClimateSystem& getInstance();
    
    void initialize();
    void update(float deltaTime);
    
    // Clima por región
    void setRegionClimate(const std::string& regionName, 
                         float avgTemperature, 
                         float avgRainfall);
    
    // Cambio climático a largo plazo
    void applyClimateChange(float temperatureDelta, float rainfallDelta);
    
    // Eventos extremos
    bool checkExtremeWeatherEvent();
    void triggerDrought(float duration);
    void triggerHeatwave(float duration);
    void triggerFlood(float duration);
    
private:
    ClimateSystem() = default;
    
    struct RegionClimate {
        float baseTemperature;
        float baseRainfall;
        float currentTemperature;
        float currentRainfall;
    };
    
    std::unordered_map<std::string, RegionClimate> regions;
    
    float globalTemperatureModifier = 0.0f;
    float globalRainfallModifier = 0.0f;
    
    // Eventos activos
    struct ActiveEvent {
        enum class Type { Drought, Heatwave, Flood, None };
        Type type = Type::None;
        float remainingDuration = 0.0f;
        float intensity = 1.0f;
    };
    
    ActiveEvent activeEvent;
};

// Efectos del clima en cultivos y animales
class WeatherEffects {
public:
    static WeatherEffects& getInstance();
    
    void update(float deltaTime);
    
    // Efectos en cultivos
    float getCropGrowthModifier(const glm::vec3& position) const;
    float getCropDamageRisk(const glm::vec3& position) const;
    bool needsIrrigation(const glm::vec3& position) const;
    bool needsProtection(const glm::vec3& position) const;
    
    // Efectos en animales
    float getAnimalComfortModifier(const glm::vec3& position) const;
    bool needsShelter(const glm::vec3& position) const;
    float getDiseaseRisk(const glm::vec3& position) const;
    
    // Efectos en suelo
    float getSoilErosionRisk(const glm::vec3& position) const;
    float getSoilMoistureGain(const glm::vec3& position) const;
    
private:
    WeatherEffects() = default;
    
    float calculateWindChill(float temperature, float windSpeed) const;
    float calculateHeatIndex(float temperature, float humidity) const;
};

} // namespace FarmEngine
