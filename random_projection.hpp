#ifndef RANDOM_PROJECTION
#define RANDOM_PROJECTION

#include <math.h>
#include <unordered_map>
#include <list>


#include "my_vector.hpp"
#include "f_i.hpp"

class random_projection{
  private:
    const unsigned int k,m,l;
    const float w;
    unsigned int new_d;
    f_i ***table_f_i;
    std::unordered_map<int, my_vector> **hash_table;

    int hash_function(my_vector x, unsigned int iteration);
  public:
    random_projection(const unsigned int _l=10, const float _w=4000,
              const unsigned int _k=4, const unsigned int _m=pow(2,32)-5);
    ~random_projection();
    void train(std::list <my_vector> *train_data_set);
    int find_NN(my_vector query);
    void find_rNN(my_vector query);
};


#endif