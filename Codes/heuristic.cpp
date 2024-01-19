#include<cmath>
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<cstring>
#include<math.h>
#include<fstream>
#include<limits.h>

#include "heuristic.hpp"
#include "EVRP.hpp"

using namespace std;


solution *best_sol;   //see heuristic.hpp for the solution structure
double initial_temperature = 2000.0;
double final_temperature = 0.1;
double cooling_rate = 0.95;
/*initialize the structure of your heuristic in this function*/
void initialize_heuristic(){

    best_sol = new solution;
    best_sol->tour = new int[NUM_OF_CUSTOMERS+1000];
    best_sol->id = 1;
    best_sol->steps = 0;
    best_sol->tour_length = INT_MAX;
}

/*implement your heuristic in this function*/
void run_random_heuristic(){

  /*generate a random solution for the random heuristic*/
  int i,help, object, tot_assigned =0;
  int *r;
  double energy_temp = 0.0; 
  double capacity_temp = 0.0;
  int from, to, temp;
  int charging_station;
  
  r = new int[NUM_OF_CUSTOMERS+1];
  //set indexes of objects
  for(i = 1; i <= NUM_OF_CUSTOMERS; i++){
    r[i-1]=i;

  }
  //randomly change indexes of objects
  for(i = 0; i <= NUM_OF_CUSTOMERS; i++){
    object = (int) ((rand()/(RAND_MAX+1.0)) * (double)(NUM_OF_CUSTOMERS-tot_assigned));
    help = r[i];
    r[i]=r[i+object];
    r[i+object]=help;
    tot_assigned++;
  }

  best_sol->steps = 0;
  best_sol->tour_length = INT_MAX;
  
  best_sol->tour[0] = DEPOT;
  best_sol->steps++;

  i = 0;
  while(i < NUM_OF_CUSTOMERS) {
    from = best_sol->tour[best_sol->steps-1];
    to = r[i];
    if((capacity_temp + get_customer_demand(to)) <= MAX_CAPACITY 
    && energy_temp+get_energy_consumption(from,to) <= BATTERY_CAPACITY)
    {
        capacity_temp  += get_customer_demand(to);
        energy_temp += get_energy_consumption(from,to);
        best_sol->tour[best_sol->steps] = to;
        best_sol->steps++;
        i++;
    } 
    else if ((capacity_temp + get_customer_demand(to)) > MAX_CAPACITY){
        capacity_temp = 0.0;
        energy_temp = 0.0;
        best_sol->tour[best_sol->steps] = DEPOT;
        best_sol->steps++;
    } else if (energy_temp+get_energy_consumption(from,to) > BATTERY_CAPACITY)
    {
       charging_station = rand() % (ACTUAL_PROBLEM_SIZE-NUM_OF_CUSTOMERS-1)+NUM_OF_CUSTOMERS+1;
       if(is_charging_station(charging_station)==true)
       {
          energy_temp = 0.0;
          best_sol->tour[best_sol->steps] =  charging_station;
          best_sol->steps++;
        }
    } else {
        capacity_temp = 0.0;
        energy_temp = 0.0;
        best_sol->tour[best_sol->steps] =  DEPOT;
        best_sol->steps++;
    }
  }
 
  //close EVRP tour to return back to the depot
   if(best_sol->tour[best_sol->steps-1]!=DEPOT){
     best_sol->tour[best_sol->steps] = DEPOT;
     best_sol->steps++;
   }

  best_sol->tour_length = fitness_evaluation(best_sol->tour, best_sol->steps);

  //free memory
  delete[] r;
}

void run_simulated_annealing()
{
  // Generate a random initial solution for the Simulated Annealing
    double temperature = 2000.0;
    double cooling_rate = 0.99;

    // Generate an initial solution using random heuristic
    run_random_heuristic();

    // Initialize the current solution with the initial solution
    int *current_solution = new int[best_sol->steps];
    memcpy(current_solution, best_sol->tour, (best_sol->steps) * sizeof(int));

    // Evaluate the fitness of the current solution
    double current_fitness = best_sol->tour_length;
    // Initialize the best solution with the current solution
    int *best_solution = new int[best_sol->steps];
    memcpy(best_solution, current_solution, (best_sol->steps) * sizeof(int));
    double best_fitness = current_fitness;

    // Perform Simulated Annealing
    while (temperature > 1.0)
    {
        // Generate a neighboring solution by swapping two random customers
        int index1 = rand() % NUM_OF_CUSTOMERS;
        int index2 = rand() % NUM_OF_CUSTOMERS;
        int temp = current_solution[index1];
        current_solution[index1] = current_solution[index2];
        current_solution[index2] = temp;

        // Evaluate the fitness of the neighboring solution
        double neighbor_fitness = fitness_evaluation(current_solution, NUM_OF_CUSTOMERS + 1);

        // Check constraints
        bool feasible = true;
        double capacity_temp = MAX_CAPACITY;
        double energy_temp = BATTERY_CAPACITY;

        for (int i = 1; i < NUM_OF_CUSTOMERS + 1; i++)
        {
            int from = current_solution[i - 1];
            int to = current_solution[i];

            // Check weight constraint
            int demand = get_customer_demand(to);
            if (capacity_temp < demand)
            {
                feasible = false;
                break;
            }
            capacity_temp -= demand;

            // Check energy constraint
            double energy_consumption = get_energy_consumption(from, to);
            if (energy_temp < energy_consumption)
            {
                feasible = false;
                break;
            }
            energy_temp -= energy_consumption;

            // Reset energy and capacity at charging stations
            if (is_charging_station(to))
            {
                capacity_temp = MAX_CAPACITY;
                energy_temp = BATTERY_CAPACITY;
            }
        }

        // Determine whether to accept the neighboring solution
        double acceptance_probability = exp((current_fitness - neighbor_fitness) / temperature);
        if (neighbor_fitness < current_fitness || (rand() / (RAND_MAX + 1.0)) < acceptance_probability)
        {
            // Accept the neighboring solution if it is feasible
            if (feasible)
            {
                current_fitness = neighbor_fitness;
            }
        }
        else
        {
            // Reject the neighboring solution and restore the current solution
            temp = current_solution[index1];
            current_solution[index1] = current_solution[index2];
            current_solution[index2] = temp;
        }

        // Update the best solution if necessary
        if (current_fitness < best_fitness)
        {
            best_fitness = current_fitness;
            memcpy(best_solution, current_solution, (best_sol->steps) * sizeof(int));
        }

        // Cool down the temperature
        temperature *= cooling_rate;
    }

    // Update the best solution to the global best solution
    best_sol->steps = best_sol->steps;
    memcpy(best_sol->tour, best_solution, (best_sol->steps) * sizeof(int));
    best_sol->tour_length = best_fitness;

    // Free memory

    delete[] current_solution;
    delete[] best_solution;
}

void run_local_search(){
  run_random_heuristic();
  bool improvement = true;
    while (improvement) {
        improvement = false;
        for (int i = 1; i < best_sol->steps - 2; i++) {
            if (best_sol->tour[i] != DEPOT || best_sol->tour[i + 1] != DEPOT)
                continue;

            for (int j = i + 2; j < best_sol->steps - 1; j++) {
                if (best_sol->tour[j] != DEPOT || best_sol->tour[j + 1] != DEPOT)
                    continue;

                // Create a copy of the current solution
                solution* temp_sol = new solution;
                temp_sol->tour = new int[NUM_OF_CUSTOMERS + 1000];
                memcpy(temp_sol->tour, best_sol->tour, sizeof(int) * (NUM_OF_CUSTOMERS + 1000));
                temp_sol->steps = best_sol->steps;
                temp_sol->tour_length = best_sol->tour_length;

                // Reverse the tour between i and j
                for (int k = i + 1, l = j; k < l; k++, l--) {
                    int temp = temp_sol->tour[k];
                    temp_sol->tour[k] = temp_sol->tour[l];
                    temp_sol->tour[l] = temp;
                }

                int new_length = fitness_evaluation(temp_sol->tour, temp_sol->steps);

                if (new_length < best_sol->tour_length) {
                    // Update the best solution with the new solution
                    delete[] best_sol->tour;
                    best_sol->tour = temp_sol->tour;
                    best_sol->steps = temp_sol->steps;
                    best_sol->tour_length = new_length;
                    improvement = true;
                } else {
                    // Free the memory for the temporary solution
                    delete[] temp_sol->tour;
                    delete temp_sol;
                }
            }
        }
    }
}
// Function to perform 2-opt local search on a solution

void run_heuristic(){
  // run_simulated_annealing();
  // run_random_heuristic();
  run_local_search();
}

/*free memory structures*/
void free_heuristic(){

  delete[] best_sol->tour;


}

