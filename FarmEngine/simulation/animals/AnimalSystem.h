#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class AnimalType {
    Chicken,
    Cow,
    Sheep,
    Pig,
    Horse,
    Duck,
    Goat,
    Rabbit
};

enum class AnimalState {
    Idle,
    Walking,
    Eating,
    Sleeping,
    Producing,
    Following
};

struct AnimalConfig {
    AnimalType type;
    std::string name;
    float moveSpeed;
    float hungerRate;
    float thirstRate;
    float happinessDecay;
    float productionTime;  // Tiempo para producir recurso
    uint32_t productID;
    uint32_t maxInventory;
    glm::vec3 size;  // Collision bounds
};

class Animal {
public:
    Animal(AnimalType type, const glm::vec3& position, uint32_t enclosureID);
    
    void update(float deltaTime);
    
    // Estado
    AnimalState getState() const { return state; }
    float getHunger() const { return hunger; }  // 0-100, 0 = hambriento
    float getThirst() const { return thirst; }
    float getHappiness() const { return happiness; }
    bool isProducing() const { return canProduce; }
    
    // Acciones del jugador
    bool feed(uint32_t foodID);
    bool water();
    bool pet();
    bool collectProduct();
    
    // Movimiento
    void setDestination(const glm::vec3& target);
    void stopMoving();
    
    // Producción
    uint32_t getProductCount() const { return products.size(); }
    const std::vector<uint32_t>& getProducts() const { return products; }
    
    // Datos
    const AnimalConfig& getConfig() const;
    uint32_t getEnclosureID() const { return enclosureID; }
    glm::vec3 getPosition() const { return position; }
    
private:
    void updateState(float deltaTime);
    void updateNeeds(float deltaTime);
    void updateMovement(float deltaTime);
    void checkProduction();
    void wander();
    
    AnimalType animalType;
    glm::vec3 position;
    glm::vec3 destination;
    uint32_t enclosureID;
    
    AnimalState state = AnimalState::Idle;
    float hunger = 100.0f;
    float thirst = 100.0f;
    float happiness = 100.0f;
    
    std::vector<uint32_t> products;
    bool canProduce = false;
    float productionTimer = 0.0f;
    
    float moveTimer = 0.0f;
    float stateTimer = 0.0f;
};

class AnimalManager {
public:
    static AnimalManager& getInstance();
    
    void initialize();
    void cleanup();
    
    // Creación
    Animal* addAnimal(AnimalType type, const glm::vec3& position, uint32_t enclosureID);
    void removeAnimal(uint32_t animalID);
    
    // Gestión por corral
    std::vector<Animal*> getAnimalsInEnclosure(uint32_t enclosureID);
    void transferAnimal(uint32_t animalID, uint32_t newEnclosureID);
    
    // Actualización
    void update(float deltaTime);
    void updateEnclosure(uint32_t enclosureID, float deltaTime);
    
    // Consultas
    Animal* getAnimalByID(uint32_t animalID);
    std::vector<Animal*> getAnimalsByType(AnimalType type);
    std::vector<Animal*> getHungryAnimals(float threshold = 50.0f);
    std::vector<Animal*> getReadyToCollect();
    
    // Estadísticas
    size_t getTotalAnimalCount() const { return animals.size(); }
    size_t getEnclosurePopulation(uint32_t enclosureID) const;
    
    // Configuración
    const AnimalConfig* getAnimalConfig(AnimalType type) const;
    
private:
    AnimalManager() = default;
    ~AnimalManager();
    
    std::unordered_map<uint32_t, std::unique_ptr<Animal>> animals;
    std::unordered_map<AnimalType, AnimalConfig> animalConfigs;
    std::unordered_map<uint32_t, std::vector<uint32_t>> enclosureAnimals;
    
    uint32_t nextAnimalID = 1;
};

// IA de animales
class AnimalAI {
public:
    struct BehaviorTree {
        enum class Node {
            Sequence,
            Selector,
            Action,
            Condition
        };
        
        Node type;
        std::vector<std::unique_ptr<BehaviorTree>> children;
        std::string actionName;
    };
    
    static AnimalAI& getInstance();
    
    void update(Animal* animal, float deltaTime);
    
    // Comportamientos
    void findFood(Animal* animal);
    void findWater(Animal* animal);
    void findRestSpot(Animal* animal);
    void avoidDanger(Animal* animal, const glm::vec3& threat);
    void followLeader(Animal* animal, const glm::vec3& leaderPos);
    
private:
    AnimalAI() = default;
    
    glm::vec3 findNearestPoint(Animal* animal, 
                               const std::vector<glm::vec3>& points);
    bool hasLineOfSight(Animal* animal, const glm::vec3& target);
};

// Sistema de cría/genética simple
class AnimalBreeding {
public:
    struct Genetics {
        float sizeModifier;      // 0.8 - 1.2
        float productionBonus;   // 0.0 - 0.5
        float diseaseResistance; // 0.0 - 1.0
        float temperament;       // 0.0 - 1.0 (calm to aggressive)
    };
    
    static AnimalBreeding& getInstance();
    
    bool canBreed(Animal* animal1, Animal* animal2);
    Animal* breed(Animal* parent1, Animal* parent2, const glm::vec3& position);
    
    const Genetics& getGenetics(uint32_t animalID) const;
    
private:
    AnimalBreeding() = default;
    
    Genetics mixGenetics(const Genetics& g1, const Genetics& g2);
    
    std::unordered_map<uint32_t, Genetics> animalGenetics;
};

} // namespace FarmEngine
