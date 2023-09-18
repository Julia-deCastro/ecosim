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

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

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
  CROW_ROUTE(app, "/next-iteration")
      .methods("GET"_method)([]()
                             {
    // Simulate the next iteration
    // Iterate over the entity grid and simulate the behaviour of each entity
    // Create a copy of the entity grid to store the updated entities
    std::vector<std::vector<entity_t>> updated_grid = entity_grid;
    
    for (uint32_t i = 0; i < NUM_ROWS; ++i) {
        for (uint32_t j = 0; j < NUM_ROWS; ++j) {
            entity_t &current_entity = entity_grid[i][j];
            entity_t &updated_entity = updated_grid[i][j];
            
            // Skip empty cells
            if (current_entity.type == empty) {
                continue;
            }
            
            // Update the age
            updated_entity.age++;
            
            // Check if the entity reaches its maximum age
            if (current_entity.type == plant && current_entity.age >= PLANT_MAXIMUM_AGE) {
                // Decompose the plant
                updated_entity.type = empty;
                updated_entity.energy = 0;
            }
            else if (current_entity.type == herbivore && current_entity.age >= HERBIVORE_MAXIMUM_AGE) {
                // Herbivore reaches its maximum age, dies
                updated_entity.type = empty;
                updated_entity.energy = 0;
            }
            else if (current_entity.type == carnivore && current_entity.age >= CARNIVORE_MAXIMUM_AGE) {
                // Carnivore reaches its maximum age, dies
                updated_entity.type = empty;
                updated_entity.energy = 0;
            } else if (current_entity.energy <= 0 && current_entity.type != plant) {
                updated_entity.type = empty;
                updated_entity.energy = 0;
                updated_entity.age = 0;
            }
            else {
                // Implement growth and additional requirements for plants
                if (current_entity.type == plant) {
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

                            // Verifica se a célula vizinha está dentro dos limites do grid
                            if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 
                                && adjacent_j < NUM_ROWS) {
                                entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                // Verifica se a célula vizinha está vazia (empty)
                                if (target_entity.type == empty) {
                                    // Cria uma nova planta na célula vizinha vazia
                                    target_entity.type = plant;
                                    target_entity.energy = 0; // A energia da planta pode ser mantida como 0
                                    target_entity.age = 0;    // A idade da planta é reiniciada
                                    break; // O crescimento da planta ocorreu com sucesso
                                }
                            }
                        }
                    }
                }

                    // Implement movement for herbivores
                    if (current_entity.type == herbivore) {
                        if ((rand() / (double)RAND_MAX) < HERBIVORE_MOVE_PROBABILITY) {
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

                                // Verifica se a célula vizinha está dentro dos limites do grid
                                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 
                                    && adjacent_j < NUM_ROWS) {
                                    entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                    // Verifica se a célula vizinha está vazia (empty) e não contém um carnívoro
                                    if (target_entity.type == empty) {
                                        // Move o herbívoro para a célula vizinha
                                        updated_entity.type = empty;
                                        updated_entity.energy = 0; // Custo de energia pelo movimento
                                        target_entity.type = herbivore;
                                        target_entity.energy = current_entity.energy - 5;
                                        target_entity.age = current_entity.age + 1;
                                        break; // O herbívoro moveu-se com sucesso
                                    }
                                }
                            }
                        }
                    }
                    
                    // Example: Implement eating for herbivores
                    if (current_entity.type == herbivore) {
                        if ((rand() / (double)RAND_MAX) < HERBIVORE_EAT_PROBABILITY) {
                            // Calcula as posições das células vizinhas (acima, abaixo, esquerda, direita)
                            std::vector<pos_t> adjacent_cells = {
                                {i - 1, j}, // Célula acima
                                {i + 1, j}, // Célula abaixo
                                {i, j - 1}, // Célula à esquerda
                                {i, j + 1}  // Célula à direita
                            };

                            // Verifica se alguma célula adjacente contém uma planta
                            for (const pos_t &adjacent_pos : adjacent_cells) {
                                uint32_t adjacent_i = adjacent_pos.i;
                                uint32_t adjacent_j = adjacent_pos.j;

                                // Verifica se a célula vizinha está dentro dos limites do grid
                                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 
                                    && adjacent_j < NUM_ROWS) {
                                    entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                    // Verifica se a célula adjacente contém uma planta
                                    if (target_entity.type == plant) {
                                        // O herbívoro come a planta
                                        updated_entity.energy += 30;
                                        current_entity.energy += 30; // Ganho de energia ao comer uma planta
                                        target_entity.type = empty; // A planta é removida
                                        target_entity.energy = 0;   // A célula fica vazia
                                        break; // O herbívoro comeu com sucesso
                                    }
                                }
                            }
                        }
                    }
                    
                    // Implement reproduction and energy update for herbivores
                    if (current_entity.type == herbivore) {
                        if (current_entity.energy > THRESHOLD_ENERGY_FOR_REPRODUCTION && 
                            (rand() / (double)RAND_MAX) < HERBIVORE_REPRODUCTION_PROBABILITY) {
                            // Verifica se a energia do herbívoro é suficiente para reprodução
                            if (current_entity.energy >= 10) {
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

                                    // Verifica se a célula vizinha está dentro dos limites do grid
                                    if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 
                                        && adjacent_j < NUM_ROWS) {
                                        entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                        // Verifica se a célula vizinha está vazia (empty)
                                        if (target_entity.type == empty) {
                                            // O herbívoro se reproduz
                                            updated_entity.energy -= 10;
                                            current_entity.energy -= 10; // Custo de energia da reprodução
                                            target_entity.type = herbivore;
                                            target_entity.energy = 20;  // Energia inicial da prole
                                            target_entity.age = 0;      // Idade da prole começa em 0
                                            break; // A reprodução do herbívoro ocorreu com sucesso
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Implement movement for carnivores
                if (current_entity.type == carnivore) {
                    if ((rand() / (double)RAND_MAX) < CARNIVORE_MOVE_PROBABILITY) {
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

                            // Verifica se a célula vizinha está dentro dos limites do grid
                            if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 && adjacent_j < NUM_ROWS) {
                                entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                // Move o carnívoro para a célula vizinha
                                updated_entity.type = empty;
                                updated_entity.energy = 0; // Custo de energia pelo movimento
                                target_entity.type = carnivore;
                                target_entity.energy = current_entity.energy - 5;
                                target_entity.age = current_entity.age + 1;
                                break; // O carnívoro moveu-se com sucesso
                            }
                        }
                    }
                }

                // Implement eating for carnivores
                if (current_entity.type == carnivore) {
                    // Verifica se alguma célula adjacente contém um herbívoro
                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            uint32_t adjacent_i = i + dx;
                            uint32_t adjacent_j = j + dy;

                            // Verifica se a célula vizinha está dentro dos limites do grid
                            if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 
                                && adjacent_j < NUM_ROWS) {
                                entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                // Verifica se a célula adjacente contém um herbívoro
                                if (target_entity.type == herbivore) {
                                    // O carnívoro come o herbívoro
                                    updated_entity.energy += 20;
                                    current_entity.energy += 20; // Ganho de energia ao comer um herbívoro
                                    target_entity.type = empty;  // O herbívoro é removido
                                    target_entity.energy = 0;    // A célula fica vazia
                                }
                            }
                        }
                    }
                }

                // Implement reproduction and energy update for carnivores
                if (current_entity.type == carnivore) {
                    if (current_entity.energy > THRESHOLD_ENERGY_FOR_REPRODUCTION && 
                        (rand() / (double)RAND_MAX) < CARNIVORE_REPRODUCTION_PROBABILITY) {
                        // Verifica se a energia do carnívoro é suficiente para reprodução
                        if (current_entity.energy >= 10) {
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

                                // Verifica se a célula vizinha está dentro dos limites do grid
                                if (adjacent_i >= 0 && adjacent_i < NUM_ROWS && adjacent_j >= 0 && adjacent_j < NUM_ROWS) {
                                    entity_t &target_entity = updated_grid[adjacent_i][adjacent_j];

                                    // Verifica se a célula vizinha está vazia (empty)
                                    if (target_entity.type == empty) {
                                        // O carnívoro se reproduz
                                        updated_entity.energy -= 10;
                                        current_entity.energy -= 10; // Custo de energia da reprodução
                                        target_entity.type = carnivore;
                                        target_entity.energy = 20;  // Energia inicial da prole
                                        target_entity.age = 0;      // Idade da prole começa em 0
                                        break; // A reprodução do carnívoro ocorreu com sucesso
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Update the entity grid with the updated entities
        entity_grid = updated_grid;
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}