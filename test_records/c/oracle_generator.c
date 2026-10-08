#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdb_tables.h"

enum { P=5040, O=729, M=9, N=P*O };
static const unsigned char src[3][7] = {
    {1,4,2,0,3,5,6}, {0,1,2,4,5,6,3}, {0,2,5,3,1,4,6}
};
static const unsigned char tw[3][7] = {
    {1,2,0,2,1,0,0}, {0,0,0,1,2,1,2}, {0,0,0,0,0,0,0}
};
static const int fac[7]={1,1,2,6,24,120,720};
static uint16_t pt[M][P], ot[M][O];

static unsigned rankp(const unsigned char p[7]) {
    unsigned r=0;
    for (int i=0;i<7;i++) {
        unsigned c=0;
        for(int j=i+1;j<7;j++) c += p[j]<p[i];
        r += c*fac[6-i];
    }
    return r;
}
static unsigned ranko(const unsigned char o[7]) {
    unsigned r=0;
    for(int i=0;i<6;i++) r=r*3+o[i];
    return r;
}
static void unrankp(unsigned r, unsigned char p[7]) {
    unsigned char available[7]={0,1,2,3,4,5,6};
    for(int i=0;i<7;i++) {
        unsigned q=r/fac[6-i]; r%=fac[6-i];
        p[i]=available[q];
        for(unsigned j=q;j<(unsigned)(6-i);j++) available[j]=available[j+1];
    }
}
static void unranko(unsigned r, unsigned char o[7]) {
    unsigned sum=0;
    for(int i=5;i>=0;i--) { o[i]=r%3; r/=3; sum+=o[i]; }
    o[6]=(3-sum%3)%3;
}
static void turn(unsigned char v[7], int face, int orientation) {
    unsigned char out[7];
    for(int i=0;i<7;i++) out[i]=orientation ? (v[src[face][i]]+tw[face][i])%3 : v[src[face][i]];
    memcpy(v,out,7);
}
static int check_pdb(int n, uint16_t tab[M][P], const uint8_t *given) {
    unsigned char d[P]; unsigned queue[P];
    memset(d,255,n); d[0]=0;
    unsigned head=0,tail=1,max=0; queue[0]=0;
    while(head<tail) {
        unsigned r=queue[head++];
        for(int m=0;m<M;m++) {
            unsigned s=tab[m][r];
            if(d[s]==255) { d[s]=d[r]+1; queue[tail++]=s; if(d[s]>max)max=d[s]; }
        }
    }
    if(tail!=(unsigned)n || memcmp(d,given,n)) return 0;
    fprintf(stdout,"PDB n=%d populated=%u max=%u solved=%u: exact match\n",n,tail,max,d[0]);
    return 1;
}
static void emit(FILE *f, const char *name, const uint16_t *tab, int n) {
    fprintf(f,".align 2\n%s:\n",name);
    for(int i=0;i<n;i++) {
        if(i%16==0) fprintf(f,"    .half ");
        fprintf(f,"%u%s",tab[i],i%16==15 || i+1==n ? "\n" : ", ");
    }
}
int main(int argc,char **argv) {
    if(argc!=2 && argc!=3) { fprintf(stdout,"Usage: generate_tables transitions.inc [oracle.bin]\n"); return 2; }
    unsigned char v[7];
    for(unsigned r=0;r<P;r++) {
        unrankp(r,v); if(rankp(v)!=r)return 1;
        for(int face=0;face<3;face++) {
            unrankp(r,v);
            for(int t=0;t<3;t++) { turn(v,face,0); pt[face*3+t][r]=rankp(v); }
        }
    }
    for(unsigned r=0;r<O;r++) {
        unranko(r,v); if(ranko(v)!=r)return 1;
        for(int face=0;face<3;face++) {
            unranko(r,v);
            for(int t=0;t<3;t++) { turn(v,face,1); ot[face*3+t][r]=ranko(v); }
        }
    }
    /* check_pdb takes a 5040-column table; copy the smaller table with that stride. */
    static uint16_t ocheck[M][P];
    for(int m=0;m<M;m++)memcpy(ocheck[m],ot[m],sizeof ot[m]);
    if(!check_pdb(P,pt,permutation_pdb) || !check_pdb(O,ocheck,orientation_pdb))return 1;

    unsigned char *d=malloc(N); unsigned *queue=malloc((size_t)N*sizeof *queue);
    if(!d || !queue)return 1;
    memset(d,255,N); d[0]=0; queue[0]=0;
    unsigned head=0,tail=1,counts[12]={1};
    while(head<tail) {
        unsigned r=queue[head++],p=r/O,o=r%O;
        if(permutation_pdb[p]>d[r] || orientation_pdb[o]>d[r])return 1;
        for(int m=0;m<M;m++) {
            unsigned s=pt[m][p]*O+ot[m][o];
            if(d[s]==255) {
                d[s]=d[r]+1; if(d[s]>11)return 1;
                counts[d[s]]++; queue[tail++]=s;
            }
        }
    }
    if(tail!=N || d[720*O]!=11)return 1;
    fprintf(stdout,"Full BFS states=%u; heuristic admissible over full domain\n",tail);
    for(int i=0;i<12;i++)fprintf(stdout,"distance %d: %u\n",i,counts[i]);
    fprintf(stdout,"21345671111111 exact distance: %u\n",d[720*O]);
    FILE *f=fopen(argv[1],"w"); if(!f)return 1;
    fprintf(f,"# Host-generated direct transitions, nine HTM moves.\n");
    for(int m=0;m<M;m++) {
        char name[32];
        sprintf(name,"perm_move_%d",m); emit(f,name,pt[m],P);
        sprintf(name,"ori_move_%d",m); emit(f,name,ot[m],O);
    }
    fclose(f);
    if(argc==3) {
        FILE *oracle=fopen(argv[2],"wb");if(!oracle)return 1;
        if(fwrite(d,1,N,oracle)!=N)return 1;
        fclose(oracle);
    }
    free(queue);free(d);return 0;
}

