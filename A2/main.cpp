#include <string>
#include <mpi.h>
#include <assert.h>
#include "randomizer.hpp"
#include <bits/stdc++.h>
using namespace std;

/*
//Notice how the randomizer is being used in this dummy function
void print_random(int tid, int num_nodes, Randomizer r){
    int this_id = tid;
    int num_steps = 10;
    int num_child = 20;

    std::string s = "Thread " + std::to_string(this_id) + "\n";
    std::cout << s;

    for(int i=0;i<num_nodes;i++){
        //Processing one node
        for(int j=0; j<num_steps; j++){
            if(num_child > 0){
                //Called only once in each step of random walk using the original node id 
                //for which we are calculating the recommendations
                int next_step = r.get_random_value(i);
                //Random number indicates restart
                if(next_step<0){
                    std::cout << "Restart \n";
                }else{
                    //Deciding next step based on the output of randomizer which was already called
                    int child = next_step % 20; //20 is the number of child of the current node
                    std::string s = "Thread " + std::to_string(this_id) + " rand " + std::to_string(child) + "\n";
                    std::cout << s;
                }
            }else{
                std::cout << "Restart \n";
            }
        }
    }
}
*/

uint32_t convertInt(uint32_t num)
{
    return (((num>>24)&0xff) | ((num<<8)&0xff0000) | ((num>>8)&0xff00) | ((num<<24)&0xff000000));
}

void constructGraph(vector<vector<uint32_t>>& graph,uint32_t num_nodes,string& graph_file)
{
    ifstream fin(graph_file,ios::in | ios::binary);
    uint32_t node1,node2;

    while(!fin.eof())
    {
        fin.read((char*)&node1,sizeof(node1));
        fin.read((char*)&node2,sizeof(node2));
        graph[convertInt(node1)].push_back(convertInt(node2));
    }
    
    fin.close();   
}


void fillRecommendations(vector<vector<pair<uint32_t,uint32_t>>>& recommend,vector<vector<uint32_t>>& graph,uint32_t num_nodes,uint32_t num_walks,uint32_t num_steps,uint32_t num_rec,uint32_t rank,uint32_t size,Randomizer r)
{
    for(uint32_t i=0;i<num_nodes;i++)
    {
        unordered_set<uint32_t> firstN;
        firstN.insert(i);

        unordered_map<uint32_t,uint32_t> scores;
        scores[i] = 0;

        for(uint32_t j=0;j<graph[i].size();i++)
        {
            uint32_t start = graph[i][j];
            firstN.insert(start);

            for(uint32_t k=0;k<num_walks;k++)
            {
                uint32_t curr = start;                
                for(uint32_t step=0;step<num_steps;step++)
                {
                    if(graph[curr].size()==0) curr = start;
                    else
                    {
                        uint32_t chance = r.get_random_value(i);
                        if(chance<0) curr = start;
                        else curr = graph[curr][chance%(graph[curr].size())]; 
                    }
                }
                scores[curr]++;
            }
        }

        priority_queue<pair<uint32_t,uint32_t>> pq;
        for(auto it = scores.begin();it!=scores.end();it++)
        {
            uint32_t node = (*it).first;
            uint32_t thisScore = (*it).second;
            
            pq.push({thisScore,node});
        }

        while(pq.size()>0)
        {
            pair<uint32_t,uint32_t> tempP = pq.top();
            pq.pop();

            if(firstN.find(tempP.second)==firstN.end())
            {
                recommend[i].push_back({tempP.second,tempP.first});
                //cout<<tempP.first<<" ";
            }
            if(recommend[i].size()==num_rec) break;
        }
        
        firstN.clear();
        scores.clear();
    }
}

int main(int argc, char* argv[]){

    assert(argc > 8);
    string graph_file = argv[1];
    uint32_t num_nodes = std::stoi(argv[2]);
    uint32_t num_edges = std::stoi(argv[3]);
    float restart_prob = std::stof(argv[4]);
    uint32_t num_steps = std::stoi(argv[5]);
    uint32_t num_walks = std::stoi(argv[6]);
    uint32_t num_rec = std::stoi(argv[7]);
    uint32_t seed = std::stoi(argv[8]);

    vector<vector<uint32_t>> graph(num_nodes);
    vector<vector<pair<uint32_t,uint32_t>>> recommend(num_nodes);
    constructGraph(graph,num_nodes,graph_file);
    
    //Only one randomizer object should be used per MPI rank, and all should have same seed
    Randomizer random_generator(seed, num_nodes, restart_prob);
    uint32_t rank, size;
    int rank_temp,size_temp;

    //Starting MPI pipeline
    MPI_Init(NULL, NULL);
    
    // Extracting Rank and Processor Count
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_temp);
    MPI_Comm_size(MPI_COMM_WORLD, &size_temp);

    rank = rank_temp;
    size = size_temp;

    fillRecommendations(recommend,graph,num_nodes,num_walks,num_steps,num_rec,rank,size,random_generator);
    //print_random(rank, num_nodes, random_generator);
    
    MPI_Finalize();
}