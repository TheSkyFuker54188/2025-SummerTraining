#include <stdio.h>

#define MAX_N 8
#define MAX_NODES 10*MAX_N			

#define pop() stack[--sp]
#define push(node) stack[sp++] = node
#define stack_not_empty (sp>0)

typedef struct state_t{
    short n, q[MAX_N];		
} state_t;

state_t stack[MAX_NODES];
int sp = 0;
int queens[MAX_N];
int conflict(int q, int p, short queens[]);
int queen(int n); 
void print_queens(state_t *st); 

int main( )
{
    state_t init_st;

    init_st.n = 0;
    push(init_st);
    queen(MAX_N);
    return 0;
}

void print_queens(state_t *st) 
{//todo
    /* for(int i = 0; i < st->n; i++)
    {
        printf("%d ", st->q[i]);
    }
    printf("\n"); */
}


int conflict(int q, int p, short queens[])
{//todo
    /* for(int i = 0; i < q; i++)
    {
        if(queens[i] == p)
        {
            return 1;
        }
    } */
    return 0;
}

int queen(int n)				
{
    int i, nCount=0;
    state_t st;

    while(stack_not_empty){
        st = pop();
        if (st.n>=n)
        {
            print_queens(&st);
            nCount ++;
            continue;
        }
        st.n++;	
        for(i=0; i<n; i++){ 
            if (conflict(st.n-1, i, st.q))
                continue;
            st.q[st.n-1] = i;	
            push(st);		
        }
    }
    printf("%d\n", nCount);
    return 0;
}
