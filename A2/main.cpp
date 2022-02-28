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
        }
    }
    file.close();
}

void fillRecommendations(vector<vector<pair<uint32_t,uint32_t>>>& recommend,vector<vector<uint32_t>>& graph,uint32_t num_nodes,uint32_t num_walks,uint32_t num_steps,uint32_t num_rec,uint32_t rank,uint32_t size,Randomizer r)
{
    int val1 = 0;
    for(uint32_t i=0;i<num_nodes;i++)
    {
        if(i%size!=rank) continue;

        unordered_set<uint32_t> firstN;
        firstN.insert(i);

        unordered_map<uint32_t,uint32_t> scores;
        scores[i] = 0;

        for(uint32_t j=0;j<graph[i].size();j++)
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

        if(recommend[i].size()>0)val1++;
    }

    uint32_t this_size = 0;
    if(this_size==0)  cout<<rank<<":"<<val1<<endl;
    
    if(rank!=0)
    {   
        for(uint32_t i=rank;i<num_nodes;i=i+size)
        {
            this_size = recommend[i].size()*2;
            MPI_Send(&this_size,1,MPI_INT,0,(int)i,MPI_COMM_WORLD);

            if(this_size!=0) MPI_Send(&recommend[i][0].first,this_size,MPI_INT,0,(int)i,MPI_COMM_WORLD);            
        }
    }
    else
    {
        int val2 = 0;
        for(uint32_t i=0;i<num_nodes;i++)
        {
            if(recommend[i].size()>0) val2++;
            if(i%size==0) continue;
            this_size = 0;

            MPI_Recv(&this_size,1,MPI_INT,i%size,(int)i,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

            if(this_size>0)
            {
                val2++;
                recommend[i].resize(this_size/2);
                MPI_Recv(&recommend[i][0].first,this_size,MPI_INT,i%size,(int)i,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

                //cout<<this_size<<"--> ";
                //for(uint32_t q=0;q<this_size/2;q++) cout<<recommend[i][q].first<<":"<<recommend[i][q].second<<" ";
                //cout<<endl;
            }
        }
        cout<<val2<<endl;
    }
}

void broadCastInput(vector<vector<uint32_t>>& graph,uint32_t num_nodes,uint32_t rank)
{
    uint32_t list_size;
    for(uint32_t i=0;i<num_nodes;i++)
    {
        list_size = graph[i].size();
        MPI_Bcast(&list_size,1,MPI_INT,0,MPI_COMM_WORLD);

        if(list_size>0 && rank==0)
        {
            MPI_Bcast(&graph[i][0],list_size,MPI_INT,0,MPI_COMM_WORLD);
        }
        else if(list_size>0)
        {
            graph[i].resize(list_size);
            MPI_Bcast(&graph[i][0],list_size,MPI_INT,0,MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char* argv[]){

    auto start = chrono::high_resolution_clock::now();

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

    cout<<rank<<": Started\n";
    
    if(rank==0) {constructGraph(graph,num_nodes,graph_file); cout<<rank<<": Graph constructed\n";}

    broadCastInput(graph,num_nodes,rank);

    if(size>0)cout<<rank<<": Graph brodcasted\n";
    
    fillRecommendations(recommend,graph,num_nodes,num_walks,num_steps,num_rec,rank,size,random_generator);

    cout<<rank<<": Recommendations constructed\n";

    if(rank==0) fillOutput(recommend,graph,num_rec);

    if(rank==0) cout<<"Output filled\n";
    
    MPI_Finalize();

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
 
    if(rank==0) cout << "Time taken "<< duration.count() << " milliseconds" << endl;
}