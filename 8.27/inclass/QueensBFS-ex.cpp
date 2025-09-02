#include <stdio.h>

#define MAX_N 8
#define MAX_NODES 10000*MAX_N			

#define put_node(st) queue[tl++]=st
#define get_node() queue[hd++]
#define queue_not_empty (hd<tl)



/*
#define pop() stack[--sp]
#define push(node) stack[sp++] = node
#define stack_not_empty (sp>0)
*/
typedef struct state_t{
    short n, q[MAX_N];		
} state_t;

state_t queue[MAX_NODES];

int hd=0, tl=0;

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
{

}

int conflict(int q, int p, short queens[])
{
    return 0;
}

int queen(int n)				
{
    int i, nCount=0;
    state_t st;

    while(queue_not_empty){
        st = get_node();
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
            put_node(st);		
        }
    }
    printf("%d\n", nCount);
    return 0;
}
