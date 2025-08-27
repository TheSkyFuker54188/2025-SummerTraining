#include <stdio.h>

#define MAX_N 8
#define MAX_NODES 10*MAX_N			//栈的大小

#define pop() stack[--sp]
#define push(node) stack[sp++] = node
#define stack_not_empty (sp>0)

typedef struct state_t{
    short n, q[MAX_N];		//n表示当前棋盘上的皇后个数；q表示当前列上皇后的行号
} state_t;

state_t stack[MAX_NODES];
int sp = 0;
int queens[MAX_N];
int conflict(int q, int p, short queens[]);//判断在(@q,@p)放置皇后是否冲突
int queen(int n); //尝试在棋盘上放置 @n 个皇后
void print_queens(state_t *st); //打印目前棋盘上的皇后

int main( )
{
    state_t init_st;

    init_st.n = 0;
    push(init_st);
    queen(MAX_N);
    return 0;
}

void print_queens(state_t *st)		//输出结果
{

}

//判断是否冲突
int conflict(int q, int p, short queens[])
{
    return 0;
}

int queen(int n)				//解决n皇后问题
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
        st.n++;	//当前棋盘上的皇后数量+1
        for(i=0; i<n; i++){ //之前的皇后与当前皇后位置i是否冲突
            if (conflict(st.n-1, i, st.q))
                continue;
            st.q[st.n-1] = i;	//不冲突，放在i行
            push(st);		//新生成的节点入栈
        }
    }
    printf("%d\n", nCount);
    return 0;
}
