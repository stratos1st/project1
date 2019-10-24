#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <string.h>
#include <bits/stdc++.h>

#include "my_curve.hpp"
#include "util.hpp"
#include "random_projection.hpp"

#define DEBUG 0

#define MAX_CURVE_POINTS 50
//TODO MAX_CURVE_POINTS from command lsh_container_size
//TODO container sz experiments

using namespace std;

int main(int argc, char *argv[]){
  //k is the k from g(x)
  //new_d is the d' from random projection aka k in arguments
  //w is the window in h
  int k=4,new_d=-1,max_points=INT_MAX,prodes=25,m=3;
  float w=0.001;
  //w not needed by projectas argument. w should be float
  double r=0.0001;
  size_t f_container_sz=200,cube_container_size=100;//not needed by project

  char input_file_data[100]("./.atomignore/trajectories_dataset_cut");
  char input_file_queries[100]("./.atomignore/queriecurve");
  char out_file[100]("curve_grid_hypercube_out");

  //------------------------------------parse arguments
  int opt;
  while((opt = getopt(argc, argv, "d:q:k:L:o:w:M:p:r:m:f:c"))!=-1){
    switch(opt){
      case 'd':
        cout<<optarg<<endl;
        strcpy(input_file_data,optarg);
        break;
      case 'q':
        strcpy(input_file_queries,optarg);
        break;
      case 'k':
        new_d=atoi(optarg);
        break;
      case 'o':
        strcpy(out_file,optarg);
        break;
      case 'w':
        w=atoi(optarg);
        break;
      case 'M':
        max_points=atoi(optarg);
        break;
      case 'p':
        prodes=atoi(optarg);
        break;
      case 'r':
        r=atoi(optarg);
        break;
      case 'm':
        m=atoi(optarg);
        break;
      case 'f':
        f_container_sz=atoi(optarg);
        break;
      case 'c':
        cube_container_size=atoi(optarg);
        break;
      default:
        cout<<"!! WRONG ARGUMENTS !!\n";
        exit(1);
    }
  }
  //------------------------------------no arguments passed
  if(argc == 1){
    std::cout << "Enter dataset path" << '\n';
    std::cin >> input_file_data; //needs manipulation
  }

  //------------------------------------create out file
  ofstream ofile;
  ofile.open(out_file);
  if(!ofile.is_open()){
    cout<<"!! ERROR OUTFILE !!\n";
    exit(1);
  }

  //------------------------------------read input files
  list <my_curve> *data1=read_curve_file(input_file_data);
  list <my_curve> *data=new list <my_curve>;
  for(auto it=data1->begin();it!=data1->end();++it)
    if(it->numofvectors<=MAX_CURVE_POINTS)
      data->push_back(*it);

  //------------------------------------create and train model
  if(new_d==-1){
    new_d=log2(data->size());
    cout<<"default d'= "<<new_d<<endl;
  }
  random_projection_curve lsh_model(MAX_CURVE_POINTS,w,k,new_d,cube_container_size,f_container_sz,m);
  lsh_model.train(data);
  cout<<"lsh training done!!\n";
  //-----------------------------------loop
  if(argc == 1){
    std::cout << "Enter new queryset path" << '\n';
    std::cin >> input_file_queries  ;
  }
  list <my_curve> *queries1=read_curve_file(input_file_queries);
  list <my_curve> *queries=new list <my_curve>;
  for(auto it=queries1->begin();it!=queries1->end();++it)
    if(it->numofvectors<=MAX_CURVE_POINTS)
      queries->push_back(*it);

  char option = 'y';
  while (option == 'y') {
    //------------------------------------info passed
    cout<<"program running with:\n\tdata_file= "<<input_file_data<<
      "\n\tquery_file= "<<input_file_queries<<"\n\tout_file= "<<out_file<<
      "\n\td'= "<<new_d<<"\n\tm= "<<m<<"\n\tM= "<<max_points<<"\n\tprodes= "<<prodes
      <<"\n\tw= "<<w<<"\n\tk= "<<k<<"\n\tr= "<<r<<"\n\tcube_container_sz= "<<
      cube_container_size<<"\n\tf_container_sz= "<<f_container_sz<<endl<<endl;

    //------------------------------------fill out file, running bruteNN and cubeNN
    double AF_max=0.0,AF_avg=0.0,AF;
    long int average_time=0;
    unsigned int total=0;
    using namespace std::chrono;
    for(list <my_curve> :: iterator it = queries->begin(); it != queries->end(); ++it){
      auto start = high_resolution_clock::now();
      pair<my_curve*,double> nn_brute=brute_NN_curve(data,*it,Dtw);
      auto stop = high_resolution_clock::now();
      auto duration_brute = duration_cast<nanoseconds>(stop - start);

      start = high_resolution_clock::now();
      pair<my_curve*,double> nn_cube=lsh_model.find_NN(*it,Dtw);
      stop = high_resolution_clock::now();
      auto duration_lsh = duration_cast<nanoseconds>(stop - start);

      if(nn_brute.second==0.0){
        cout<<"Warining: real distance is ~=0! querry "<<it->id<<" does not count\n";
        continue;
      }
      if(nn_cube.second==DBL_MAX){
        cout<<"Warining: predicted distance is ~=inf! querry "<<it->id<<" does not count\n";
        continue;
      }

      AF=nn_cube.second/nn_brute.second;
      AF_max=max(AF,AF_max);
      AF_avg+=AF;
      average_time+=duration_lsh.count();

      ofile<<"Query: "<<it->id<<endl;
      ofile<<"Nearest neighbor: "<<nn_cube.first->id<<endl;
      ofile<<"distanceCube: "<<nn_cube.second<<endl;
      ofile<<"distanceTrue: "<<nn_brute.second<<endl;
      ofile<<"tCube: "<<duration_lsh.count()<<endl;
      ofile<<"tTrue: "<<duration_brute.count()<<endl;
      ofile<<"R-near neighbors: "<<endl;

      // list<my_curve*>* rNNs=lsh_model.find_rNN(*it,r,Dtw);
      // for(list <my_curve*>::iterator it = rNNs->begin(); it != rNNs->end(); ++it)
      //   ofile<<(*it)->id<<endl;
      // rNNs->clear();
      // delete rNNs;

      total++;
    }

    AF_avg/=total;
    average_time/=total;
    cout<<"AF_max= "<<AF_max<<"\tAF_avg= "<<AF_avg<<"\taverage_time= "<<average_time<<" nanoseconds\n";
    //------------------------------------rerunning the program

    std::cout << "Would you like to run LSH for another queryset (y/n)" << '\n';
    std::cin >> option;
    while(option != 'y' && option != 'n')
      std::cin >> option;
    if(option == 'y'){
      std::cout << "Enter new queryset path" << '\n';
      std::cin >> input_file_queries;
      queries->clear();
      delete queries;
      queries1->clear();//added
      delete queries1;//added
      list <my_curve> *queries1=read_curve_file(input_file_queries);
      list <my_curve> *queries=new list <my_curve>;
      for(auto it=queries1->begin();it!=queries1->end();++it)
        if(it->numofvectors<=MAX_CURVE_POINTS)
          queries->push_back(*it);
    }
  }
  //------------------------------------clearing memmory
  ofile.close();
  data->clear();
  queries->clear();
  delete data;
  delete queries;
  queries1->clear();//added
  delete queries1;//added
  return 0;
}