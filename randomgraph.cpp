#include <bits/stdc++.h>
using namespace std;

int main(void) {
    int n, k;
    cout<< "Enter value of k, n: ";
    cin>> k>> n;

    int** matrix = new int*[n];
    for(int i = 0; i < n; ++i)
        matrix[i] = new int[n];

    srand(time(NULL));
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++)
            matrix[i][j] = 0;
    }

    long int r = rand()%(n*n)+n;    //minimum n edges(just for complex graphs)
    for(long int i=0; i<r; i++) {
        int x, y;
        while(1) {
            x=rand()%n;
            y=rand()%n;

            if(x!=y)
                break;
        }
        matrix[x][y] = 1;
        matrix[y][x] = 1;
    }

    ofstream output("input_params.txt");
    output<< k<< " "<< n<< endl;
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            output<< matrix[i][j];
            if(j!=n-1)
                output<< " ";
        }
        if(i!=n-1)
            output<<endl;
    }
}