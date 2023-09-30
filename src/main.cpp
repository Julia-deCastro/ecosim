#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

struct position {
    int row;
    int col;
};


std::mutex entity_mtx;
// Grid que contém as entidades
static std::vector<std::vector<entity_t>> entity_grid;
// Vetor para armazenar as posições já analisadas
std::vector<position> analyzedPositions;

void simulate_entity_plant(uint32_t i, uint32_t j) {
    entity_mtx.lock();
    if (entity_grid[i][j].age >= PLANT_MAXIMUM_AGE) {
        // Decompor a planta
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    } else {
        entity_grid[i][j].age++;
        if ((rand() / (double)RAND_MAX) < PLANT_REPRODUCTION_PROBABILITY) {
            // Calcula as posições das células vizinhas (acima, abaixo, esquerda, direita)
            std::vector<pos_t> adjacent_cells = {
                {i - 1, j}, // Célula acima
                {i + 1, j}, // Célula abaixo
                {i, j - 1}, // Célula à esquerda
                {i, j + 1}  // Célula à direita
            };

            // Embaralha aleatoriamente as posições das células vizinhas
            std::random_shuffle(adjacent_cells.begin(), adjacent_cells.end());

            for (const pos_t &adjacent_pos : adjacent_cells) {
                uint32_t adjacent_i = adjacent_pos.i;
                uint32_t adjacent_j = adjacent_pos.j;
                position tempPosition;
                tempPosition.row = adjacent_i;
                tempPosition.col = adjacent_j;

                // Verifica se a célula vizinha está dentro dos limites do grid
                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0
                    && adjacent_j < NUM_ROWS) {
                    entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];

                    // Verifica se a célula vizinha está vazia (empty)
                    if (target_entity.type == empty) {
                        // Cria uma nova planta na célula vizinha vazia
                        target_entity.type = plant;
                        target_entity.energy = 0; // A energia da planta pode ser mantida como 0
                        target_entity.age = 0;    // A idade da planta é reiniciada
                        analyzedPositions.push_back(tempPosition);
                        break; // O crescimento da planta ocorreu com sucesso
                    }
                }
            }
        }
    }
    entity_mtx.unlock();
}

void simulate_entity_herbivore(uint32_t i, uint32_t j) {
    entity_mtx.lock();
    std::vector<pos_t> adjacent_cells = {
        {i - 1, j}, // Célula acima
        {i + 1, j}, // Célula abaixo
        {i, j - 1}, // Célula à esquerda
        {i, j + 1}  // Célula à direita
    };

    if (entity_grid[i][j].age >= HERBIVORE_MAXIMUM_AGE || entity_grid[i][j].energy <= 0) {
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    } else {
        entity_grid[i][j].age++;
        if ((rand() / (double)RAND_MAX) < HERBIVORE_EAT_PROBABILITY) {
            // Verifica se alguma célula adjacente contém uma planta
            for (const pos_t &adjacent_pos : adjacent_cells) {
                uint32_t adjacent_i = adjacent_pos.i;
                uint32_t adjacent_j = adjacent_pos.j;
                position tempPosition;
                tempPosition.row = adjacent_i;
                tempPosition.col = adjacent_j;

                // Verifica se a célula vizinha está dentro dos limites do grid
                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 &&
                    adjacent_j < NUM_ROWS) {
                    entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];

                    // Verifica se a célula adjacente contém uma planta
                    if (target_entity.type == plant) {
                        // O herbívoro come a planta
                        entity_grid[i][j].energy += 30; // Ganho de energia ao comer uma planta
                        target_entity.type = empty;    // A planta é removida
                        target_entity.energy = 0;      // A célula fica vazia
                        analyzedPositions.push_back(tempPosition);
                        break; // O herbívoro comeu com sucesso
                    }
                }
            }
        }

        if (entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION &&
            (rand() / (double)RAND_MAX) < HERBIVORE_REPRODUCTION_PROBABILITY) {
            // Verifica se a energia do herbívoro é suficiente para reprodução
            if (entity_grid[i][j].energy >= 10) {
                // Embaralha aleatoriamente as posições das células vizinhas
                std::random_shuffle(adjacent_cells.begin(), adjacent_cells.end());

                for (const pos_t &adjacent_pos : adjacent_cells) {
                    uint32_t adjacent_i = adjacent_pos.i;
                    uint32_t adjacent_j = adjacent_pos.j;
                    position tempPosition;
                    tempPosition.row = adjacent_i;
                    tempPosition.col = adjacent_j;
                    // Verifica se a célula vizinha está dentro dos limites do grid
                    if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 &&
                        adjacent_j < NUM_ROWS) {
                        entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];

                        // Verifica se a célula vizinha está vazia (empty)
                        if (target_entity.type == empty) {
                            // O herbívoro se reproduz
                            entity_grid[i][j].energy -= 10;
                            target_entity.type = herbivore;
                            target_entity.energy = 20;     // Energia inicial da prole
                            target_entity.age = 0;         // Idade da prole começa em 0
                            analyzedPositions.push_back(tempPosition);
                            break; // A reprodução do herbívoro ocorreu com sucesso
                        }
                    }
                }
            }
        }
        if ((rand() / (double)RAND_MAX) < HERBIVORE_MOVE_PROBABILITY) {
            // Embaralha aleatoriamente as posições das células vizinhas
            std::random_shuffle(adjacent_cells.begin(), adjacent_cells.end());

            for (const pos_t &adjacent_pos : adjacent_cells) {
                uint32_t adjacent_i = adjacent_pos.i;
                uint32_t adjacent_j = adjacent_pos.j;

                // Verifica se a célula vizinha está dentro dos limites do grid
                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 &&
                    adjacent_j < NUM_ROWS) {
                    entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];
                    position tempPosition;
                    tempPosition.row = adjacent_i;
                    tempPosition.col = adjacent_j;
                    // Verifica se a célula vizinha está vazia (empty) e não contém um carnívoro
                    if (target_entity.type == empty) {
                        // Move o herbívoro para a célula vizinha
                        target_entity.type = herbivore;
                        target_entity.energy = entity_grid[i][j].energy - 5;
                        target_entity.age = entity_grid[i][j].age;
                        entity_grid[i][j].type = empty;
                        entity_grid[i][j].energy = 0;
                        entity_grid[i][j].age = 0;
                        analyzedPositions.push_back(tempPosition);
                        break; // O herbívoro moveu-se com sucesso
                    }
                }
            }
        }
    }
    entity_mtx.unlock();
}

void simulate_entity_carnivore(uint32_t i, uint32_t j) {
    entity_mtx.lock();
    std::vector<pos_t> adjacent_cells = {
        {i - 1, j}, // Célula acima
        {i + 1, j}, // Célula abaixo
        {i, j - 1}, // Célula à esquerda
        {i, j + 1}  // Célula à direita
    };

    if (entity_grid[i][j].age >= CARNIVORE_MAXIMUM_AGE || entity_grid[i][j].energy <= 0) {
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    } else {
        entity_grid[i][j].age++;
        // Verifica se alguma célula adjacente contém um herbívoro
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                uint32_t adjacent_i = i + dx;
                uint32_t adjacent_j = j + dy;
                position tempPosition;
                tempPosition.row = adjacent_i;
                tempPosition.col = adjacent_j;

                // Verifica se a célula vizinha está dentro dos limites do grid
                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 && adjacent_j < NUM_ROWS) {
                    entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];

                    // Verifica se a célula adjacente contém um herbívoro
                    if (target_entity.type == herbivore) {
                        // O carnívoro come o herbívoro
                        entity_grid[i][j].energy += 20; // Ganho de energia ao comer um herbívoro
                        target_entity.type = empty;    // O herbívoro é removido
                        target_entity.energy = 0;      // A célula fica vazia
                        target_entity.age = 0;
                        analyzedPositions.push_back(tempPosition);
                        break;
                    }
                }
            }
        }

        if (entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION && 
            (rand() / (double)RAND_MAX) < CARNIVORE_REPRODUCTION_PROBABILITY) {
            // Verifica se a energia do carnívoro é suficiente para reprodução
            if (entity_grid[i][j].energy >= 10) {
                // Embaralha aleatoriamente as posições das células vizinhas
                std::random_shuffle(adjacent_cells.begin(), adjacent_cells.end());

                for (const pos_t &adjacent_pos : adjacent_cells) {
                    uint32_t adjacent_i = adjacent_pos.i;
                    uint32_t adjacent_j = adjacent_pos.j;
                    position tempPosition;
                    tempPosition.row = adjacent_i;
                    tempPosition.col = adjacent_j;

                    // Verifica se a célula vizinha está dentro dos limites do grid
                    if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 && adjacent_j < NUM_ROWS) {
                        entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];

                        // Verifica se a célula vizinha está vazia (empty)
                        if (target_entity.type == empty) {
                            // O carnívoro se reproduz
                            entity_grid[i][j].energy -= 10; // Custo de energia da reprodução
                            target_entity.type = carnivore;
                            target_entity.energy = 20;      // Energia inicial da prole
                            target_entity.age = 0;          // Idade da prole começa em 0
                            analyzedPositions.push_back(tempPosition);
                            break; // A reprodução do carnívoro ocorreu com sucesso
                        }
                    }
                }
            }
        }

        if ((rand() / (double)RAND_MAX) < CARNIVORE_MOVE_PROBABILITY) {
            // Embaralha aleatoriamente as posições das células vizinhas
            std::random_shuffle(adjacent_cells.begin(), adjacent_cells.end());

            for (const pos_t &adjacent_pos : adjacent_cells) {
                uint32_t adjacent_i = adjacent_pos.i;
                uint32_t adjacent_j = adjacent_pos.j;

                // Verifica se a célula vizinha está dentro dos limites do grid
                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 && adjacent_j < NUM_ROWS) {
                    entity_t &target_entity = entity_grid[adjacent_i][adjacent_j];
                    position tempPosition;
                    tempPosition.row = adjacent_i;
                    tempPosition.col = adjacent_j;
                    // Verifica se a célula vizinha está vazia (empty) e não contém um carnívoro
                    if (target_entity.type == empty) {
                        // Move o herbívoro para a célula vizinha
                        target_entity.type = carnivore;
                        target_entity.energy = entity_grid[i][j].energy - 5;
                        target_entity.age = entity_grid[i][j].age;
                        entity_grid[i][j].type = empty;
                        entity_grid[i][j].energy = 0; // Custo de energia pelo movimento
                        entity_grid[i][j].age = 0;
                        analyzedPositions.push_back(tempPosition);
                        break; // O herbívoro moveu-se com sucesso
                    }
                }
            }
        }
    }
    entity_mtx.unlock();
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        int i;
        int row, col;
        for(i = 0; i < (uint32_t)request_body["plants"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = plant;
            entity_grid[row][col].age = 0; 
        }
        for(i = 0; i < (uint32_t)request_body["herbivores"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = herbivore;
            entity_grid[row][col].age = 0;
            entity_grid[row][col].energy = 100; 
        }
        for(i = 0; i < (uint32_t)request_body["carnivores"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = carnivore;
            entity_grid[row][col].age = 0;
            entity_grid[row][col].energy = 100;
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

  // Endpoint to process HTTP GET requests for the next simulation iteration
        CROW_ROUTE(app, "/next-iteration").methods("GET"_method)([]() {
        bool pos_analyzed = false;
        for (uint32_t i = 0; i < NUM_ROWS; i++) {
            for (uint32_t j = 0; j < NUM_ROWS; j++) {
                // verifica se a posição do grid já foi analizada
                bool analyzed_pos = false;
                for (const position& analyzedPos : analyzedPositions) {
                if (analyzedPos.row == i && analyzedPos.col == j) {
                    analyzed_pos = true;
                    break;
                }
            }

                if (!analyzed_pos) {
                    // Cria uma thread para simular a entidade nessa posição
                    if (entity_grid[i][j].type == plant) {
                        std::thread palnt_thread(simulate_entity_plant, i, j);
                        palnt_thread.join();
                    }
                    else if (entity_grid[i][j].type == herbivore) {
                        std::thread herbivore_thread(simulate_entity_herbivore, i, j);
                        herbivore_thread.join();
                    }
                    else if (entity_grid[i][j].type == carnivore) {
                        std::thread carnivore_thread(simulate_entity_carnivore, i, j);
                        carnivore_thread.join();
                    }
                }
            }
        }
        analyzedPositions.clear();
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid;
        return json_grid.dump();
    });
    app.port(8080).run();

    return 0;
}