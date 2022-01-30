#include "psort.h"
#include <omp.h>
#include <bits/stdc++.h>
using namespace std;


void merge(uint32_t *arr1,uint32_t size1,uint32_t *arr2,uint32_t size2,uint32_t* newarr)
{
    uint32_t i1,i2,j;
    i1 = 0;
    i2 = 0;
    j = 0;

    while(i1<size1 && i2<size2)
    {
        if(arr1[i1]<arr2[i2])
        {
            newarr[j] = arr1[i1];
            i1++;
        }
        else
        {
            newarr[j] = arr2[i2];
            i2++;
        }
        j++;
    }

    while(i1<size1)
    {
        newarr[j] = arr1[i1];
        j++;
        i1++;
    }

    while(i2<size2)
    {
        newarr[j] = arr2[i2];
        j++;
        i2++;
    }
}

void sequentialSort(uint32_t *arr,uint32_t size)
{
    if(size<=1) return;
    sequentialSort(arr,size/2);
    sequentialSort(arr+size/2,size - size/2);

    uint32_t* newarr = new uint32_t[size];
    merge(arr,size/2,arr+size/2,size-size/2,newarr);

    for(uint32_t i=0;i<size;i++) arr[i] = newarr[i];

    return;
}

void ParallelSort(uint32_t *data, uint32_t n, int p)
{
    if(n<=1) return;
    if(n<100) {sequentialSort(data,n); return;}

    uint32_t *R = new uint32_t[p*p];

    for(int i=0;i<p;i++)
    {
        for(int j=0;j<p;j++)
        {
            R[j+i*p] = data[j + i*(n/p)];
        }
    }

    sequentialSort(R,p*p);

    uint32_t *S = new uint32_t[p-1];                            // values at partition
    uint32_t *pointers = new uint32_t[n];                       // pointer to bin number
    //uint32_t** binSizes = new uint32_t*[p];                   // size of this bin in this thread

    uint32_t* binSizes = new uint32_t[p];
    uint32_t* prevSizes = new uint32_t[p];                      // total size of bins including this one
    uint32_t* finaloffset = new uint32_t[p];                    // offset achived yet
    uint32_t* newData = new uint32_t[n];                        // where newData is stored

    for(int i=0;i<p;i++)
    {
        if(i<p-1) S[i] = R[(i+1)*p];
        binSizes[i] = 0;
        prevSizes[i] = 0;
        finaloffset[i] = 0;
    }

    for(int k=0;k<p;k++)
    {
        int location = k;
        #pragma omp task firstprivate(data,location)
        {
            for(uint32_t i=0;i<n;i++)
            {
                if((location==p-1 && S[p-2]<data[i]) || (location!=p-1 && location>0 && data[i]<=S[location] && data[i]>S[location-1]) || (location==0 && data[i]<=S[0]))
                {
                    pointers[i] = location;
                    binSizes[location]++;
                }
            }
        }
    }

    /*
    for(uint32_t i=0;i<n;i++)
    {
        uint32_t thisVal = data[i];
        uint32_t location = i;

        #pragma omp task firstprivate(location,thisVal)
        {
            for(int j=0;j<p-1;j++)
            {
                if(thisVal<=S[j])
                {
                    pointers[location] = j;
                    binSizes[j][omp_get_thread_num()]++;
                    break;
                }
            }
            if(S[p-2]<thisVal)
            {
                pointers[location] = p-1;
                binSizes[p-1][omp_get_thread_num()]++;
            }
        }
    }*/

    #pragma omp taskwait    
    

    for(int i=0;i<p;i++)
    {
        prevSizes[i] = binSizes[i];
        if(i>0) prevSizes[i] += prevSizes[i-1];
        //cout<<prevSizes[i]<<" ";
    }
    //cout<<"\n";
    //return;

    uint32_t totaloffset;
    for(uint32_t i=0;i<n;i++)
    {
        if(pointers[i]>0) totaloffset = prevSizes[pointers[i]-1] + finaloffset[pointers[i]];
        else totaloffset = finaloffset[pointers[i]];

        newData[totaloffset] = data[i];

        finaloffset[pointers[i]]++;  
    }

    //#pragma omp parallel for
    for(uint32_t i=0;i<n;i++) data[i] = newData[i];

    for(int i=0;i<p;i++)
    {
        #pragma omp task firstprivate(i)
        {
            if(n==0 && prevSizes[0]>(2*n/p)) ParallelSort(data,prevSizes[0],p);
            else if(n==0)sequentialSort(data,prevSizes[0]);
            else if((prevSizes[i] - prevSizes[i-1])>(2*n/p)) ParallelSort(data+prevSizes[i-1],prevSizes[i] - prevSizes[i-1],p);
            else sequentialSort(data+prevSizes[i-1],prevSizes[i] - prevSizes[i-1]);
        }
    }

    return;
}