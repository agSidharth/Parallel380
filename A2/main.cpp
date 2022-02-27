#include <string>
#include <mpi.h>
#include <assert.h>
#include "randomizer.hpp"
#include <bits/stdc++.h>
using namespace std;

void constructGraph(vector<vector<uint32_t>>& graph,uint32_t num_nodes,string& graph_file)
{
    ifstream fin(graph_file,ios::in | ios::binary);
    uint32_t node1,node2;

    while(!fin.eof())
    {
        fin.read((char*)&node1,sizeof(node1));
        fin.read((char*)&node2,sizeof(node2));
        graph[__builtin_bswap32(node1)].push_back(__builtin_bswap32(node2));
    }
    
    fin.close();   
}


void fillOutput(vector<vector<pair<uint32_t,uint32_t>>>& recommend,vector<vector<uint32_t>>& graph,uint32_t num_rec)
{
    ofstream file("output.dat",ios::out | ios::binary);

    for(uint32_t i=0;i<recommend.size();i++)
    {
        uint32_t temp = __builtin_bswap32(graph[i].size());
        file.write((char *)&temp,sizeof(temp));

        for(uint32_t j=0;j<recommend[i].size();j++)
        {
            temp = __builtin_bswap32(recommend[i][j].first);
            file.write((char *)&temp,sizeof(temp));

            temp = __builtin_bswap32(recommend[i][j].second);
            file.write((char *)&temp,sizeof(temp));
        }
        if(recommend[i].size()<num_rec)
        {
            file.write((char *)&("NULL"),sizeof("NULL")); 
            // REMEMBER:: is null also stored in bigendian
        }
    }
    file.close();
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

    fillOutput(recommend,graph,num_rec);
    //print_random(rank, num_nodes, random_generator);
    
    MPI_Finalize();
}