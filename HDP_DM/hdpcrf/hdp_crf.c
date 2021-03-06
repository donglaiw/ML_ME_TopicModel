#ifndef _HDP_CRF
#define _HDP_CRF

#define HDP_EPSILON 0.00001

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dp_crf.c"
#include "base_crf.c"
#include "util_crf.c"
#include "conparam_crf.c"
#ifndef _HDP
#define hdp_deleteclass(array,start,size,type) { \
  type *pp, *pe, tmpvar; \
  pp     = array + start; \
  pe     = array + size; \
  tmpvar = *pp; \
  while ( pp < pe ) { *pp = *(pp+1); pp++; } \
  *pe = tmpvar; \
}
#endif
typedef struct {
    int numdp, numconparam;
    BASE_C *base;
    DP_C *dp;
    CONPARAM *conparam;
    int *dpstate, *ppindex, *cpindex, *ttindex;
    } HDP_C;
/*
 * Structure for hierarchical Dirichlet process mixtures with crf (t,k) representation.
 * same with the original structure HDP except DP_C, BASE_C instead of DP,BASE
 * 1) I/O:
 * void Readgamln(double *gamln_0,double *gamln_a,double *gamln_g,double *gamln_l,double *gamln_wl,int tmp)
 *              Reads the precomputed gammaln(n),gammaln(n+alpha),gammaln(n+gamma),
 *              gammaln(n+lambda),gammaln(n+W*lambda), where n=0:1:tmp
 * hdp *Readtxt(int init)
 *              Reads in a HDP_C from files written by mat2txt.m in matlab.
 *              init=1: read *.txt
 *              init!=1: read *_c.txt
 * void Writetxt(HDP_C *hdp, int init)
 *              Writes a HDP_C to files for txt2mat.m in matlab.Frees memory allocated.
 *              init=1: write *_c.txt
 *              init!=1: write *_d.txt
 * 2) Auxilary Manipulation:
 * void hdp_c_relabeltable(DP_C *dp)
 *              Remove empty tables and relabel table assignment
 * void hdp_c_oldval(BASE_C *base, DP_C *dp_1,int *cltindex)
 *              New Reconfiguration is worse and return to the back-up previous configuration
 * void hdp_c_newval(BASE_C *base, DP_C *dp_1,int *cltindex)
 *              New Reconfiguration is better and update the back-up configuration
 * void hdp_c_relabelclass(BASE_C *base, DP_C *alldp)
 *              Remove empty topics and relabel topic assignment
 * 3) Search Moves:
 * 3.1) low-level move:
 * double hdp_c_localdatatt(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res,int ww)
 *              Randomly go through all the customers in Restaurant res and assign them to the best table
 *              ww=-1: used in Decompose_Restaurant move
 *              ww!=-1: used in Decompose_Word move, for efficiency, only go through customers with word ww
 * double hdp_c_localtablecc(BASE_C *base, DP_C *dp_1, DP_C *dp, int res, int exclude)
 *              Randomly go through all the tables in Restaurant res and assign them to the best dish
 *              except the excluded dish
 * double hdp_c_mergetable(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res, int exclude)
 *              Randomly go through all the tables in Restaurant res, merge one to another,
 *              and find the best dish for them except the excluded dish
 * double hdp_c_lmres(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res, int exclude,int ww,int init)
 *              Iterate through the above 3 moves until convergence
 *
 * 3.2) mid-level move:
 * double hdp_c_deleteres(BASE_C *base, DP_C *dp_1, DP_C *dp, int res, double T)
 *              Delete restaurant res configuration
 * double hdp_c_randres(BASE_C *base, DP_C *dp_1, DP_C *dp , int freezecl, int res, double T)
 *              Resample restaurant res configuration given others fixed
 * double hdp_c_deleteword(BASE_C *base, DP_C *alldp, int word, int cl, int **wodd, int *ress, double T)
 *              Delete word configuration in topic cl
 * double hdp_c_randword(BASE_C *tmp_base, DP_C *tmp_alldp, int word, int cl, int **wodds, int *ress, double T)
 *              Resample word configuration in topic cl
 *
 * 3.3) high-level move:
 * double hdp_c_decompres(BASE_C *base, DP_C *dp_1, double T, int freezecl, int *cltindex, int all,int init)
 * double hdp_c_decompword(BASE_C *base, DP_C *alldp, double T, int cl, int all)
 * double hdp_c_decompclass(BASE_C *base, DP_C *alldp, double T, int cl, int all,int init)
 * double hdp_c_mergeclass(HDP_C* hdp, double T,int mergecl)
 *
 */

/*1)  I/O*/
void Readgamln(double *gamln_0,double *gamln_a,double *gamln_g,double *gamln_l,double *gamln_wl,int tmp) {
    FILE *file;
    file = fopen( "./test/IO_goodbar_50_01/gamln.txt", "r" );
    int ii,ff;
    for(ii=0; ii<tmp; ii++) {
        ff=fscanf( file, "%lf", &gamln_0[ii]);
        }

    for(ii=0; ii<tmp; ii++) {
        ff=fscanf( file, "%lf", &gamln_a[ii]);
        }

    for(ii=0; ii<tmp; ii++) {
        ff=fscanf( file, "%lf", &gamln_g[ii]);
        }
    /*
       for(ii=0; ii<tmp; ii++) {
           ff=fscanf( file, "%lf", &gamln_l[ii]);
           }
    */
    for(ii=0; ii<tmp; ii++) {
        ff=fscanf( file, "%lf", &gamln_wl[ii]);
        }
    fclose(file);
    }

HDP_C *Readtxt(int init) {
    int ii, jj,kk,ff;
    FILE *file;
    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/hdp.txt", "r" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/hdp_c.txt", "r" );
        }

    /*HDP general*/
    HDP_C *hdp;
    hdp           = malloc(sizeof(HDP_C));
    ff=fscanf( file, "%d", &hdp->numdp);
    ff=fscanf( file, "%d", &hdp->numconparam);
    hdp->dpstate=malloc(sizeof(int)*hdp->numdp);
    hdp->ppindex=malloc(sizeof(int)*hdp->numdp);
    hdp->cpindex=malloc(sizeof(int)*hdp->numdp);
    hdp->ttindex=malloc(sizeof(int)*hdp->numdp);
    for(ii=0; ii<hdp->numdp; ii++) {
        ff=fscanf( file, "%d", &hdp->dpstate[ii]);
        }
    for(ii=0; ii<hdp->numdp; ii++) {
        ff=fscanf( file, "%d", &hdp->ppindex[ii]);
        }
    for(ii=0; ii<hdp->numdp; ii++) {
        ff=fscanf( file, "%d", &hdp->cpindex[ii]);
        }
    for(ii=0; ii<hdp->numdp; ii++) {
        ff=fscanf( file, "%d", &hdp->ttindex[ii]);
        }
    fclose(file);


    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/base.txt", "r" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/base_c.txt", "r" );
        }
    BASE_C *base;
    int maxclass,numclass,tmp,numdp;
    hdp->base           = malloc(sizeof(BASE_C));
    base=hdp->base;
    ff=fscanf( file, "%d", &numclass );
    base->numclass=numclass;
    base->lastcl=numclass-1;
    base->o_numclass=numclass;
    base->o_lastcl=numclass-1;

    base->maxclass = maxclass = (base->numclass+2) * 10;
    base->maxta=numclass;
    base->o_maxta=numclass;
    base->hh=malloc(sizeof(HH)*maxclass);
    ff=fscanf( file, "%d", &base->numword);

    for (ii=0; ii<numclass; ii++) {
        base->hh[ii].eta=malloc(sizeof(double)*(base->numword+1));
        base->hh[ii].eta[0]=0;
        for (jj=1; jj<base->numword+1; jj++) {
            ff=fscanf( file, "%lf", &(base->hh[ii].eta[jj]));
            base->hh[ii].eta[0]+=base->hh[ii].eta[jj];
            }
        }
    base->avg_h         = base->hh[0].eta[0]/base->numword;
    base->noiselevel    = 0.5*base->avg_h;
    for (ii=numclass; ii<maxclass; ii++) {
        base->hh[ii].eta=malloc(sizeof(double)*(base->numword+1));
        for (jj=1; jj<base->numword+1; jj++) {
            base->hh[ii].eta[jj]=base->avg_h;
            }
        base->hh[ii].eta[0]=base->hh[0].eta[0];
        }

    /*for faster computation*/

    QQ classnd=malloc(sizeof(int)*maxclass);
    memset(classnd,0,sizeof(int)*maxclass);
    for (jj=0; jj<numclass+1; jj++) {
        ff=fscanf( file, "%d", &(classnd[jj]));
        }

    QQ *classqq=malloc(sizeof(QQ)*maxclass);
    for (jj=0; jj<numclass+1; jj++) {
        classqq[jj]=malloc(sizeof(int)*(base->numword+1));
        classqq[jj][0]=0;
        for (ii=1; ii<base->numword+1; ii++) {
            ff=fscanf( file, "%d", &(classqq[jj][ii]));
            }
        }
    for (jj=numclass+1; jj<maxclass; jj++) {
        classqq[jj]=malloc(sizeof(int)*(base->numword+1));
        memset(classqq[jj],0,sizeof(int)*(base->numword+1));
        }

    base->classqq=ReadSparse(classqq,base->numword,numclass, maxclass,0,1,base->hh);

    base->wordqq=RightConnect(base->classqq,base->numword,numclass);
    base->cclik    = malloc(sizeof(double)*maxclass);
    memset(base->cclik,0,sizeof(double)*maxclass);

    for (jj=0; jj<numclass+1; jj++) {
        ff=fscanf( file, "%lf", &base->cclik[jj]);
        }

    base->o_cclik    = malloc(sizeof(double)*maxclass);
    memcpy(base->o_cclik,base->cclik,sizeof(double)*maxclass);

    base->lastcl   = numclass-1;

    QQ *ctindex=malloc(sizeof(QQ)*maxclass);
    numdp=hdp->numdp;
    for (jj=0; jj<numclass+1; jj++) {
        ctindex[jj]=malloc(sizeof(int)*numdp);
        for (ii=0; ii<numdp; ii++) {
            ff=fscanf( file, "%d", &ctindex[jj][ii]);
            }
        }
    for (jj=numclass+1; jj<maxclass; jj++) {
        ctindex[jj]=malloc(sizeof(int)*numdp);
        memset(ctindex[jj],0,sizeof(int)*numdp);
        }
    base->ctindex=ReadSparse(ctindex, numdp-1,numclass, maxclass,0,2,NULL);

    base->numdp    = hdp->numdp;

    fclose(file);


    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/dp.txt", "r" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/dp_c.txt", "r" );
        }
    DP_C *dp;
    hdp->dp=malloc(sizeof(DP_C)*hdp->numdp);

    QQ* tabless=malloc(sizeof(QQ)*maxclass);
    for (jj=0; jj<maxclass; jj++) {
        tabless[jj]=malloc(sizeof(int)*(base->numword+1));
        }

    for(ii=0; ii<hdp->numdp; ii++) {
        dp=  hdp->dp+ii;
        ff=fscanf( file, "%d", &dp->numtable);
        if(ii!=0) {
            numclass=dp->numtable;
            dp->lastta=dp->numtable-1;
            dp->o_lastta=dp->numtable-1;
            }
        dp->o_numtable=dp->numtable;
        for (jj=0; jj<numclass+1; jj++) {
            ff=fscanf( file, "%d", &classnd[jj]);
            }
        ff=fscanf( file, "%lf", &dp->alpha);
        ff=fscanf( file, "%lf", &dp->ttlik);
        dp->o_ttlik=dp->ttlik;
        ff=fscanf( file, "%d", &dp->numdata);
        if(ii!=0) {
            dp->datass=malloc(sizeof(int)*dp->numdata);
            for (jj=0; jj<dp->numdata; jj++) {
                ff=fscanf( file, "%d", &dp->datass[jj]);
                }
            dp->datatt=malloc(sizeof(int)*dp->numdata);
            for (jj=0; jj<dp->numdata; jj++) {
                ff=fscanf( file, "%d", &dp->datatt[jj]);
                }
            dp->o_datatt=malloc(sizeof(int)*dp->numdata);
            memcpy(dp->o_datatt,dp->datatt,sizeof(int)*dp->numdata);
            dp->tablecc=malloc(sizeof(int)*maxclass);
            memset(dp->tablecc,0,sizeof(int)*maxclass);
            for (jj=0; jj<dp->numtable+1; jj++) {
                ff=fscanf( file, "%d", &dp->tablecc[jj]);
                }
            dp->o_tablecc=malloc(sizeof(int)*maxclass);
            memcpy(dp->o_tablecc,dp->tablecc,sizeof(int)*maxclass);

            for (jj=0; jj<numclass+1; jj++) {
                tabless[jj][0]=0;
                for (kk=1; kk<base->numword+1; kk++) {
                    ff=fscanf( file, "%d", &(tabless[jj][kk]));
                    }
                }
            for (jj=numclass+1; jj<maxclass; jj++) {
                memset(tabless[jj],0,sizeof(int)*(base->numword+1));
                }
            /*need the gl for each node now*/
            dp->tabless=ReadSparse(tabless, base->numword,numclass, maxclass,0,0,NULL);
            dp->wordss=RightConnect(dp->tabless,base->numword,dp->numtable);
            }

        }

    fclose(file);


    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/conparam.txt", "r" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/conparam_c.txt", "r" );
        }
    CONPARAM *conparam;
    hdp->conparam=malloc(hdp->numconparam*sizeof(CONPARAM));
    for(ii=0; ii<hdp->numconparam; ii++) {
        conparam=hdp->conparam+ii;
        ff=fscanf( file, "%lf", &conparam->alphaa);
        ff=fscanf( file, "%lf", &conparam->alphab);
        ff=fscanf( file, "%d", &conparam->numdp);
        ff=fscanf( file, "%lf", &conparam->alpha);
        conparam->totalnd=malloc(conparam->numdp*sizeof(int));
        conparam->totalnt=malloc(conparam->numdp*sizeof(int));
        for(jj=0; jj<conparam->numdp; jj++) {
            ff=fscanf( file, "%d", &conparam->totalnd[jj]);
            }
        for(jj=0; jj<conparam->numdp; jj++) {
            ff=fscanf( file, "%d", &conparam->totalnt[jj]);
            }
        }

    fclose(file);

    /**/

    for (jj=0; jj<maxclass; jj++) {
        free(tabless[jj]);
        free(classqq[jj]);
        free(ctindex[jj]);
        }
    free(tabless);
    free(classnd);
    free(ctindex);
    free(classqq);
    return hdp;
    }

void Writetxt(HDP_C *hdp,int init) {
    int ii, jj,kk;
    FILE *file;
    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/hdp_c.txt", "w" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/hdp_d.txt", "w" );
        }

    /*HDP general*/
    fprintf( file, "%d\n", hdp->numdp);
    fprintf( file, "%d\n", hdp->numconparam);
    for(ii=0; ii<hdp->numdp; ii++) {
        fprintf( file, "%d ", hdp->dpstate[ii]);
        }
    fprintf( file, "\n");
    for(ii=0; ii<hdp->numdp; ii++) {
        fprintf( file, "%d ", hdp->ppindex[ii]);
        }
    fprintf( file, "\n");
    for(ii=0; ii<hdp->numdp; ii++) {
        fprintf( file, "%d ", hdp->cpindex[ii]);
        }
    fprintf( file, "\n");
    for(ii=0; ii<hdp->numdp; ii++) {
        fprintf( file, "%d ", hdp->ttindex[ii]);
        }
    fprintf( file, "\n");
    free(hdp->dpstate);
    free(hdp->ppindex);
    free(hdp->cpindex);
    free(hdp->ttindex);
    fclose(file);


    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/base_c.txt", "w" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/base_d.txt", "w" );
        }
    BASE_C *base;
    base=hdp->base;
    int maxclass=base->maxclass,numclass=base->numclass,*tmp,tmp2,numdp;
    int hahaha=numclass;

    fprintf( file, "%d\n", numclass );
    fprintf( file, "%d\n", base->numword);


    for (ii=0; ii<numclass; ii++) {
        for (jj=1; jj<base->numword+1; jj++) {
            fprintf( file, "%lf ", (base->hh[ii].eta[jj]));
            }
        fprintf( file, "\n");

        }

    QQ *classnd,*classqq,*tabless,*ctindex;
    classnd=WriteSparse(1,base->numclass+1,0,base->classqq,base->classqq,0,0);
    classqq=WriteSparse(base->numword,base->numclass+1,base->maxclass,base->classqq,base->wordqq,0,2);
    for (jj=0; jj<numclass+1; jj++) {
        fprintf( file, "%d ", classnd[0][jj]);
        }
    fprintf( file, "\n");
    free(classnd[0]);
    free(classnd);

    for (jj=0; jj<numclass+1; jj++) {
        for (ii=1; ii<base->numword+1; ii++) {
            fprintf( file, "%d ", (classqq[jj][ii]));
            }
        fprintf( file, "\n");

        }

    for (jj=0; jj<numclass+1; jj++) {
        fprintf( file, "%lf ", base->cclik[jj]);
        }
    fprintf( file, "\n");


    classnd=WriteSparse(1,base->numclass+1,0,base->ctindex,base->classqq,0,0);
    ctindex=WriteSparse(base->numdp-1,base->numclass+1,base->maxclass,base->ctindex,base->ctindex,0,1);


    for (jj=0; jj<numclass+1; jj++) {
        fprintf( file, "%d ", classnd[0][jj]);
        for (ii=1; ii<base->numdp; ii++) {
            fprintf( file, "%d ", ctindex[jj][ii]);
            }
        fprintf( file, "\n");
        }
    DeleteBase_C(base,1);
    fclose(file);


    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/dp_c.txt", "w" );
        }
    else {
        file = fopen( "./test/IO_goodbar_50_01/dp_d.txt", "w" );
        }
    DP_C *dp;
    for(ii=0; ii<hdp->numdp; ii++) {
        dp=  hdp->dp+ii;
        fprintf( file, "%d\n", dp->numtable);
        if(ii!=0) {
            numclass=dp->numtable;
            classnd=WriteSparse(1,dp->numtable+1,dp->numtable,dp->tabless,dp->tabless,0,0);
            }

        for (jj=0; jj<numclass+1; jj++) {
            fprintf( file, "%d ", classnd[0][jj]);
            }

        free(classnd[0]);
        free(classnd);

        fprintf( file, "\n");
        fprintf( file, "%lf\n", dp->alpha);
        fprintf( file, "%lf\n", dp->ttlik);
        fprintf( file, "%d\n", dp->numdata);
        if(ii!=0) {
            for (jj=0; jj<dp->numdata; jj++) {
                fprintf( file, "%d ", dp->datass[jj]);
                }
            free(dp->datass);
            fprintf( file, "\n");
            for (jj=0; jj<dp->numdata; jj++) {
                fprintf( file, "%d ", dp->datatt[jj]);
                }
            fprintf( file, "\n");
            free(dp->datatt);
            free(dp->o_datatt);
            for (jj=0; jj<dp->numtable+1; jj++) {
                fprintf( file, "%d ", dp->tablecc[jj]);
                }
            fprintf( file, "\n");
            free(dp->tablecc);
            free(dp->o_tablecc);

            tabless=WriteSparse(base->numword,dp->numtable+1,maxclass,dp->tabless,dp->wordss,0,2);
            for (jj=0; jj<numclass+1; jj++) {
                for (kk=1; kk<base->numword+1; kk++) {
                    fprintf( file, "%d ", (tabless[jj][kk]));
                    }
                fprintf( file, "\n");
                }

            for (jj=0; jj<numclass+1; jj++) {
                free(tabless[jj]);
                }
            free(tabless);

            }
        }
    free(hdp->dp);

    fclose(file);

    if(init==1) {
        file = fopen( "./test/IO_goodbar_50_01/conparam_c.txt", "w" );
        }
    else {
        file = fopen( "./IO_goodbar_50_01/conparam_d.txt", "w" );
        }
    CONPARAM *conparam;
    for(ii=0; ii<hdp->numconparam; ii++) {
        conparam=hdp->conparam+ii;
        fprintf( file, "%lf\n", conparam->alphaa);
        fprintf( file, "%lf\n", conparam->alphab);
        fprintf( file, "%d\n", conparam->numdp);
        fprintf( file, "%lf\n", conparam->alpha);

        for(jj=0; jj<conparam->numdp; jj++) {
            fprintf( file, "%d ", conparam->totalnd[jj]);
            }
        fprintf( file, "\n");
        for(jj=0; jj<conparam->numdp; jj++) {
            fprintf( file, "%d ", conparam->totalnt[jj]);
            }
        fprintf( file, "\n");
        free(conparam->totalnd);
        free(conparam->totalnt);
        }
    free(hdp->conparam);
    fclose(file);
    /**/

    free(base);
    free(hdp);


    for (jj=0; jj<hahaha+1; jj++) {
        free(classqq[jj]);
        free(ctindex[jj]);
        }
    free(classqq);
    free(ctindex);

    }

/*2)  Auxilary Manipulation*/

void hdp_c_relabeltable(DP_C *dp) {
    int i, j, tmp,*tablecc, *datatt, *ind,lastta;
    OLink *tabless,*wordss;
    tablecc           = dp->tablecc;
    tabless           = dp->tabless;
    wordss            = dp->wordss;
    datatt            = dp->datatt;
    lastta            = dp->lastta;
    /*start from backward*/
    tmp               = lastta;
    ind               = malloc(sizeof(int)*(1+lastta));
    for(i=lastta; i>=0; i--) {
        /*the last one is empty on purpose*/
        ind[i]         = i;
        if(tabless[i]==NULL||tabless[i]->val==0) {
            if(tabless[i]!=NULL) {
                /*wordss: right connection:(update() in newval has taken care of it)*/
                DelNode(tabless[i]);
                tabless[i]=NULL;
                }
            hdp_deleteclass(tablecc, i, tmp, int);
            hdp_deleteclass(tabless, i, tmp, OLink);
            if(tabless[tmp]!=NULL) {
                DelNode(tabless[tmp]);
                }
            tmp   -= 1;
            for(j=i+1; j<=lastta; j++) {
                ind[j]--;
                }
            }
        }
    dp->lastta=dp->numtable-1;

    if(tmp!= lastta) {
        /*relabel datatt*/
        for(i=0; i<dp->numdata; i++) {
            datatt[i]=ind[datatt[i]];
            }
        /*relabel tabless->col*/
        tabless=dp->tabless;
        OLNode *p;
        for(i=0; i<dp->numtable; i++) {
            p=tabless[i]->down;
            if(p->col!=i) {
                SetCol(tabless[i],i);
                }
            }
        }
    free(ind);
    }

void hdp_c_oldval(BASE_C *base, DP_C *dp_1,int *cltindex) {
    int ress,res,last;
    DP_C *dp;
    OLink *tabless;
    for(ress=0; ress<cltindex[0]; ress++) {
        res                 = cltindex[ress+1];
        dp                  = dp_1+1+res;
        last=(dp->lastta>dp->o_lastta)?dp->lastta:dp->o_lastta;
        Revert(dp->wordss,dp->tabless,base->numword,last+1,0);
        tabless=dp->tabless;
        for(last=dp->o_lastta+1; last<=dp->lastta; last++) {
            if(tabless[last]!=NULL) {
                DelNode(tabless[last]);
                tabless[last]=NULL;
                }
            }
        memcpy(dp->datatt,dp->o_datatt,sizeof(int)*dp->numdata);
        memcpy(dp->tablecc,dp->o_tablecc,sizeof(int)*base->maxclass);
        dp->numtable=dp->o_numtable;
        dp->ttlik   = dp->o_ttlik;
        dp->lastta  = dp->o_lastta;
        }

    dp_1->numtable=dp_1->o_numtable;
    dp_1->ttlik   = dp_1->o_ttlik;
    last=(base->lastcl>base->o_lastcl)?base->lastcl:base->o_lastcl;
    base->numclass=base->o_numclass;
    base->maxta=base->o_maxta;
    memcpy(base->cclik,base->o_cclik,sizeof(double)*base->maxclass);
    base->lastcl  = base->o_lastcl;
    Revert(base->wordqq,base->classqq,base->numword,last+1,1);
    Revert(base->wordqq,base->ctindex,base->numdp,last+1,2);/**/
    }

void hdp_c_newval(BASE_C *base, DP_C *dp_1,int *cltindex) {
    int ress,res,last;
    DP_C *dp;
    OLink *classqq;
    for(ress=0; ress<cltindex[0]; ress++) {
        res                 = cltindex[ress+1];
        dp                  = dp_1+1+res;
        last=(dp->lastta>dp->o_lastta)?dp->lastta:dp->o_lastta;
        Update(dp->wordss,dp->tabless,base->numword,last+1,0);
        hdp_c_relabeltable(dp);
        memcpy(dp->o_datatt,dp->datatt,sizeof(int)*dp->numdata);
        memcpy(dp->o_tablecc,dp->tablecc,sizeof(int)*base->maxclass);
        dp->o_numtable=dp->numtable;
        dp->o_ttlik   = dp->ttlik;
        dp->o_lastta  = dp->lastta;
        }
    dp_1->o_numtable=dp_1->numtable;
    dp_1->o_ttlik   = dp_1->ttlik;
    base->o_numclass=base->numclass;
    base->o_maxta=base->maxta;
    memcpy(base->o_cclik,base->cclik,sizeof(double)*base->maxclass);
    Update(base->wordqq,base->classqq,base->numword,base->lastcl+1,1);
    Update(base->wordqq,base->ctindex,base->numdp,base->lastcl+1,2);
    classqq=base->classqq;
    while(classqq[base->lastcl]->val==0) {
        base->lastcl--;
        }

    base->o_lastcl  = base->lastcl;
    }

void hdp_c_relabelclass(BASE_C *base, DP_C *alldp) {
    int i, j, k, numclass, lastcl, tmp, *classnd, *ind, count;
    int *classnt, *tablecc;
    OLink *classqq, *ctindex;
    OLNode *p;
    OLNode *q;
    DP_C *dp, *dp_1;
    dp_1              = &alldp[0];
    double *cclik;
    HH *hh;
    numclass          = base->numclass;
    lastcl            = base->lastcl;
    cclik             = base->cclik;
    classqq           = base->classqq;
    ctindex           = base->ctindex;
    hh                = base->hh;
    /*start from backward*/
    tmp               = lastcl;
    ind               = malloc(sizeof(int)*(lastcl+1));

    for(i=lastcl; i>=0; i--) {
        ind[i]         = i;
        if(classqq[i]->val==0) {
            //printf("del  %d\n",i);
            /*shift element one step forward*/
            hdp_deleteclass(cclik, i, tmp, double);
            hdp_deleteclass(hh, i, tmp, HH);
            hdp_deleteclass(classqq, i, tmp, OLink);
            hdp_deleteclass(ctindex, i, tmp, OLink);
            /*set the last element 0*/

            if(ctindex[tmp]==NULL) {
                printf("sth. wrong with ctindex during relabel ...\n");
                }
            ctindex[tmp]->val=0;
            ctindex[tmp]->o_val=0;
            ctindex[tmp]->down=NULL;
            if(classqq[tmp]==NULL) {
                printf("sth. wrong with classqq during relabel ...\n");
                }
            classqq[tmp]->val=0;
            classqq[tmp]->o_val=0;
            classqq[tmp]->down=NULL;

            for(j=1; j<base->numword+1; j++) {
                base->hh[tmp].eta[j]=base->avg_h;
                }
            base->hh[tmp].eta[0]=base->hh[0].eta[0];
            /**/
            tmp   -= 1;
            ind[i] = -1;
            for(j=i+1; j<=lastcl; j++) {
                ind[j]--;
                }
            }
        }
    /*caveat...*/
    cclik[numclass]=0;
    memcpy(base->o_cclik,base->cclik,sizeof(double)*base->maxclass);
    /*relable tablecc,classqq,ctindex*/
    base->lastcl=numclass-1;
    base->o_lastcl=numclass-1;
    if(tmp!= lastcl) {
        for(i=0; i<=lastcl; i++) {
            if(ind[i]>-1 && ind[i]!=i) {
                p=ctindex[ind[i]]->down;
                for(j=0; j<ctindex[ind[i]]->val; j++) {
                    dp    = alldp+p->row+1;
                    p=p->down;
                    tablecc=dp->tablecc;
                    for(k=0; k<dp->numtable; k++) {
                        if(tablecc[k]==i) {
                            tablecc[k]=ind[i];
                            /*there may not be only one such table in the restaurant*/
                            }
                        }
                    memcpy(dp->o_tablecc,dp->tablecc,sizeof(int)*base->maxclass);
                    if(p==NULL) {
                        break;
                        }
                    }
                }
            }
        }
    for(i=0; i<numclass; i++) {
        p=classqq[i]->down;
        q=ctindex[i]->down;
        if(p->col!=i) {
            SetCol(classqq[i],i);
            }
        if(q->col!=i) {
            SetCol(ctindex[i],i);
            }
        }
    /*no need for wordqq since no word is attached to the deleted class */
    /*base->wordqq=RightConnect(base->classqq,base->numword,numclass);*/
    /*new val*/
    /*
    Update(base->wordqq,base->classqq,base->numword,base->lastcl+1,1);
    Update(base->wordqq,base->ctindex,base->numdp,base->lastcl+1,2);
    int *none = malloc(sizeof(int)*base->numdp);
    none[0]             = base->numdp-1;
        for(i=0; i<base->numdp-1; i++) {
            none[i+1]=i;
            }
    hdp_c_newval(base, alldp,none);
    free(none);
    */
    free(ind);

    }


/*3)  Search Moves*/

/*3.1) Base Local moves*/

/*input: no two tables with same dish, otherwise a little fussy to manage tabless*/
double hdp_c_localdatatt(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res,int ww) {
    /*0) variables to be read in from struct */
    int numclass, maxclass, lastcl, numtable, numdata, totalnt;
    int *datass, *datatt, *tablecc;
    OLink *classqq,*wordqq, *tabless,*wordss, *ctindex;
    HH *hh;
    double ttlik, ttlik1, *cclik, alpha, gamma, tmp;
    /* from dp */
    numdata  = dp->numdata;
    datass   = dp->datass;
    datatt   = dp->datatt;
    numtable = dp->numtable;
    tablecc  = dp->tablecc;
    tabless  = dp->tabless;
    wordss   = dp->wordss;
    ttlik    = dp->ttlik;
    alpha    = dp->alpha;
    /* from dp_1 */
    gamma    = dp_1->alpha;
    totalnt  = dp_1->numtable;
    ttlik1   = dp_1->ttlik;
    /* from base */
    classqq  = base->classqq;
    wordqq  = base->wordqq;
    numclass = base->numclass;
    maxclass = base->maxclass;
    cclik    = base->cclik;
    hh       = base->hh;
    ctindex  = base->ctindex;
    lastcl   = base->lastcl;
    /* temp variables */
    int ii, jj, ss, oldtt, oldcc, searchtt, searchcc, tablepo, classpo,wnum;
    double del_tt, del_tt1, del_cc, tablelik[2], classlik[2], tmplik[2], tmpbest, eta, etasum, oldtablelik, newclasslik,  prelik, del_F=0;
    /* for search use */
    int prenumta=dp->lastta+1;
    etasum=hh[0].eta[0];
    int pre_ss=-1,*fullrow,*fullrow2;
    fullrow=malloc(sizeof(int)*(lastcl+1));
    fullrow2=malloc(sizeof(int)*maxclass);
    OLink ha;

    int *order;
    order=malloc(sizeof(int)*(numdata));
    randperm(numdata,order);
    int iii;
    for(iii=0; iii<numdata; iii++) {
        ii=order[iii];
        ss                     = datass[ii];
        if(ww==-1||ww==ss) {
            if(pre_ss!=ss) {
                ExpandRow(wordqq[ss-1],fullrow,lastcl);
                ExpandRow(wordss[ss-1],fullrow2,prenumta);
                }
            oldtt                  = datatt[ii];
            oldcc                  = tablecc[oldtt];
            del_tt1                = 0;

            eta                    = hh[oldcc].eta[ss];
            /* 1) remove data item from model*/
            /*remove from base*/
            wnum=AddElement(classqq[oldcc],wordqq[ss-1],ss-1,oldcc,-1,1,hh[oldcc].eta);
            fullrow[oldcc]        -= 1;
            fullrow2[oldtt]       -= 1;
            del_cc                 = log(classqq[oldcc]->val+etasum)-log(wnum+eta);
            /*remove from dp*/
            if(tabless[oldtt]->val==1) {
                del_tt              = -log(alpha);
                numtable           -= 1;
                /*remove from dp_1*/
                totalnt            -= 1;
                del_tt1            += log(totalnt+gamma);
                if(ctindex[oldcc]->val==1) {
                    del_tt1         -= log(gamma);
                    numclass        -=1;
                    }
                else {
                    del_tt1         -= log(ctindex[oldcc]->val-1);
                    }
                }
            else {
                del_tt              = -log(tabless[oldtt]->val-1);
                }
            prelik                 = T*del_tt+del_tt1+del_cc;
            /*new counting term*/
            del_tt                 += log(fullrow2[oldtt]+1);
            /* 2) search new config*/
            /* 2.1) search old table*/
            tablepo                  =-1;
            classpo                  =-1;
            memset(tmplik, 0, 2*sizeof(double));
            tmpbest=-prelik;
            for(jj=0; jj<prenumta; jj++) {
                if(tabless[jj]!=NULL&&tabless[jj]->val!=0 && jj!=oldtt) {
                    searchcc            = tablecc[jj];
                    eta                 = hh[searchcc].eta[ss];
                    /*new counting term*/
                    tmplik[0]           = log(tabless[jj]->val)-log(fullrow2[jj]+1);
                    tmplik[1]           = log(eta+fullrow[searchcc])-log(classqq[searchcc]->val+etasum);
                    if(T*tmplik[0]+tmplik[1]-tmpbest>HDP_EPSILON) {
                        memcpy(tablelik, tmplik, sizeof(double)*2);
                        tablepo          = jj;
                        tmpbest          = T*tablelik[0]+tablelik[1];
                        }
                    }
                }
            /* 2.2) search new table with the best dish */
            /*if previously was a new table, no need for the following
             * if(0!=0) {        */
            if(tabless[oldtt]->val!=1) {
                tmp                 = T*log(alpha)-log(totalnt+gamma);
                memset(tmplik, 0, 2*sizeof(double));
                /*a little hacky, but it makes no sense to make a single customer own a new dish*/
                for(jj=0; jj<=lastcl; jj++) {
                    if(classqq[jj]->val!=0&&jj!=oldcc) {
                        eta               = hh[jj].eta[ss];
                        tmplik[0]         = log(ctindex[jj]->val);
                        /*new table will have the new term 0*/
                        tmplik[1]         = -log(classqq[jj]->val+etasum)+log(eta+fullrow[jj]);
                        if(tmp+tmplik[0]+tmplik[1]-tmpbest>HDP_EPSILON) {
                            memcpy(classlik, tmplik, sizeof(double)*2);
                            classpo        = jj;
                            tablepo        = -1;
                            tmpbest        = tmplik[0]+tmplik[1]+tmp;
                            }
                        }
                    }
                }
            /* 3) decision: */
            if(classpo!=-1 || tablepo!=-1) {
                /*3.1) new config*/
                wnum=AddElement(tabless[oldtt],wordss[ss-1],ss-1,oldtt,-1,0,NULL);
                /*update ctindex*/
                if(tabless[oldtt]->val==0) {
                    wnum=AddColElement(ctindex[oldcc],res,oldcc,-1);
                    }
                if(tablepo!=-1) {
                    /*3.1.1) old table*/
                    /*update base*/
                    classpo              = tablecc[tablepo];
                    wnum=AddElement(classqq[classpo],wordqq[ss-1],ss-1,classpo,1,1,hh[classpo].eta);
                    cclik[oldcc]        += del_cc;
                    cclik[classpo]      += tablelik[1];
                    del_F               += del_cc+tablelik[1];
                    /*update dp*/
                    wnum=AddElement(tabless[tablepo],wordss[ss-1],ss-1,tablepo,1,0,NULL);
                    ttlik               += del_tt+tablelik[0];
                    /*tablecc[oldtt]       = 0;*/
                    datatt[ii]           = tablepo;
                    fullrow2[tablepo]   += 1;
                    /*update dp_1*/
                    ttlik1              += del_tt1;
                    }
                else {
                    /*3.1.2) new table (previous table cannot be empty) */
                    /*update dp*/
                    ttlik               += del_tt+log(alpha);
                    numtable            += 1;
                    datatt[ii]           = prenumta;
                    /*hack: wont create new class, space is enough*/
                    if(tabless[prenumta]==NULL) {
                        tabless[prenumta]    = malloc(sizeof(OLNode));
                        tabless[prenumta]->down =NULL;
                        tabless[prenumta]->val=0;
                        tabless[prenumta]->o_val=0;
                        }
                    wnum=AddElement(tabless[prenumta],wordss[ss-1],ss-1,prenumta,1,0,NULL);
                    tablecc[prenumta]    = classpo;
                    fullrow2[prenumta]        = 1;
                    prenumta            += 1;
                    /*update dp_1*/
                    ttlik1              += del_tt1+classlik[0]-log(totalnt+gamma);
                    totalnt             += 1;
                    wnum=AddColElement(ctindex[classpo],res,classpo,1);
                    /*update base*/
                    wnum=AddElement(classqq[classpo],wordqq[ss-1],ss-1,classpo,1,1,hh[classpo].eta);
                    cclik[oldcc]        += del_cc;
                    cclik[classpo]      += classlik[1];
                    del_F               += del_cc+classlik[1];

                    }
                fullrow[classpo]        += 1;
                }
            else {
                /*3.2 restore previous config*/
                /*update dp*/
                if(tabless[oldtt]->val==1) {
                    numtable        += 1;
                    /*update dp_1*/
                    totalnt         += 1;
                    if(ctindex[oldcc]->val==1) {
                        numclass     +=1;
                        }
                    }
                /*update base*/
                wnum=AddElement(classqq[oldcc],wordqq[ss-1],ss-1,oldcc,1,1,hh[oldcc].eta);
                fullrow[oldcc]        += 1;
                fullrow2[oldtt]       += 1;
                }
            pre_ss=ss;
            }

        }
    free(fullrow);
    free(fullrow2);
    /*4. update the non-pointers*/
    del_F                  += T*(ttlik-dp->ttlik)+ttlik1-dp_1->ttlik;
    /*dp*/
    dp->numtable            = numtable;
    dp->ttlik               = ttlik;
    dp->lastta              = prenumta-1;
    /*dp1*/
    dp_1->ttlik             = ttlik1;
    dp_1->numtable          = totalnt;
    /*base*/
    base->numclass          = numclass;
    /*5.   Relabel tables (relabel classes is costly and will do later)*/
    free(order);
    return del_F;
    }

/*output: no two tables with same dish, otherwise a little fussy to manage tabless*/
double hdp_c_localtablecc(BASE_C *base, DP_C *dp_1, DP_C *dp, int res, int exclude) {
    /*0) variables to be read in from struct */
    int numclass, lastcl, maxcl, numtable;
    int *datass, *datatt, *tablecc, *tablend ;
    int *classnt, *classnd;
    OLink *classqq,*wordqq, *tabless,*wordss, *ctindex;
    HH *hh;
    double ttlik, ttlik1, *cclik, alpha, gamma, prelik, del_F=0;
    int wnum;
    /* from dp */
    tablecc  = dp->tablecc;
    tabless  = dp->tabless;
    wordss  = dp->wordss;
    alpha    = dp->alpha;
    numtable = dp->numtable;
    /* from dp_1 */
    gamma    = dp_1->alpha;
    ttlik1   = dp_1->ttlik;
    /* from base */
    classqq  = base->classqq;
    wordqq   = base->wordqq;
    numclass = base->numclass;
    cclik    = base->cclik;
    hh       = base->hh;
    ctindex  = base->ctindex;
    lastcl   = (base->lastcl>base->o_lastcl)?base->lastcl:base->o_lastcl;
    maxcl    = base->maxclass;
    /* temp variables */
    int ii, jj, kk, ss, oldcc, classpo;
    double etasum, del_tt1, del_cc, tmplik[2], tmpbest, oldclasslik, oldttlik1;
    etasum=hh[0].eta[0];
    double *del_col=malloc(sizeof(double)*(1+base->numword));
    double *tmp_del=malloc(sizeof(double)*(1+base->numword));
    double *copy_del=malloc(sizeof(double)*(1+base->numword));

    int *order;
    order=malloc(sizeof(int)*(dp->lastta+1));
    randperm(dp->lastta+1,order);
    int iii;
    for(iii=0; iii<dp->lastta+1; iii++) {
        ii=order[iii];
        if(tabless[ii]!=NULL&&tabless[ii]->val!=0) {
            oldcc                  = tablecc[ii];
            /* 1) remove table item from model*/
            /*remove from base*/
            if(classqq[oldcc]->val!=tabless[ii]->val) {
                del_cc              = delta_k(classqq[oldcc],tabless[ii],hh[oldcc].eta,-1,del_col);
                del_tt1             = -log(ctindex[oldcc]->val-1);
                }
            else {
                del_cc              = -cclik[oldcc];
                del_tt1             = -log(gamma);
                numclass           -= 1;
                for(ss=1; ss<=base->numword; ss++) {
                    del_col[ss]=gamln(0,hh[oldcc].eta[ss],4);
                    }
                del_col[0]=gamln(0,hh[0].eta[0],5);
                }
            /* 2) search new config*/
            /* 2.1) search old dish*/
            classpo                = -1;
            oldclasslik            = -del_cc;
            oldttlik1              = -del_tt1;
            for(jj=0; jj<=lastcl; jj++) {
                if(classqq[jj]!=NULL&&classqq[jj]->val!=0&&oldcc!=jj&&jj!=exclude) {
                    tmplik[0]            = delta_k(classqq[jj],tabless[ii],hh[jj].eta, 1,tmp_del);
                    tmplik[1]            = log(ctindex[jj]->val);
                    if(tmplik[0]+tmplik[1]-oldclasslik-oldttlik1>HDP_EPSILON) {
                        oldclasslik    = tmplik[0];
                        oldttlik1      = tmplik[1];
                        classpo        = jj;
                        memcpy(copy_del,tmp_del,sizeof(double)*(1+base->numword));
                        }
                    }
                }
            /* 2.2) new dish*/
            if(exclude!=-3) {
                tmpbest=delta_k(NULL,tabless[ii],hh[maxcl-1].eta, 0,tmp_del);
                }
            else {
                /*no new dish*/
                tmpbest=-log(gamma)+oldclasslik+oldttlik1-10;
                }
            if(tmpbest+log(gamma)-oldclasslik-oldttlik1>HDP_EPSILON) {
                classpo             = lastcl+1;
                oldttlik1           = log(gamma);
                oldclasslik         = tmpbest;
                memcpy(copy_del,tmp_del,sizeof(double)*(1+base->numword));
                }
            /* 3) decision: */
            if(classpo!=-1) {
                /*3.1) new config*/
                /*update dp*/
                tablecc[ii]          = classpo;
                /*update dp_1*/
                ttlik1              += del_tt1+oldttlik1;
                /*update base*/
                DeleteCol(classqq[oldcc], tabless[ii],1,del_col);
                /*update ctindex*/
                wnum=AddColElement(ctindex[oldcc],res,oldcc,-1);
                cclik[oldcc]        += del_cc;
                del_F               += del_cc+oldclasslik;
                /*3.2) if new class added*/
                if(classpo>lastcl) {
                    //printf("New dish added from lc %d,%f,%f,%f,%f\n",lastcl,del_cc,oldclasslik,del_tt1,oldttlik1);
                    numclass         += 1;
                    lastcl           += 1;
                    if(lastcl+1 > maxcl) {
                        printf("need reinitialize maxclass.....\n");
                        printf("we design the sparse matrix to be array of list for faster i/o\n");
                        printf("we don't reallocate the sparse matrix\n");
                        printf("sry about the bad design......\n");
                        /*alloc more space
                        hdp_c_resizeclass(dp_1, base);
                        classqq       = base->classqq;
                        cclik         = base->cclik;
                        ctindex       = base->ctindex;
                        maxcl         = base->maxclass;
                        classnd       = base->classnd;
                        classnt       = dp_1->classnd;
                        dp            = dp_1+res+1;
                        tablend       = dp->classnd;
                        tablecc  = dp->tablecc;
                        tabless  = dp->tabless;*/
                        }
                    if(ctindex[classpo]==NULL) {
                        ctindex[classpo]=malloc(sizeof(OLNode));
                        ctindex[classpo]->down=NULL;
                        ctindex[classpo]->val=0;
                        ctindex[classpo]->o_val=0;
                        /**/

                        }
                    if(classqq[classpo]==NULL) {
                        classqq[classpo]=malloc(sizeof(OLNode));
                        classqq[classpo]->down=NULL;
                        classqq[classpo]->val=0;
                        classqq[classpo]->o_val=0;
                        /*
                        */
                        }
                    cclik[classpo]      = oldclasslik;
                    }
                else {
                    cclik[classpo]      += oldclasslik;
                    }
                wnum=AddColElement(ctindex[classpo],res,classpo,1);
                AddCol(classqq[classpo], tabless[ii],wordqq,classpo,1,copy_del);
                }
            else {
                /*3.2 restore previous config*/
                /*restore base*/
                if(classqq[oldcc]->val==tabless[ii]->val) {
                    numclass           += 1;
                    }
                }
            }
        }
    /*4. Relabel classes later*/
    /*5. update the non-pointers*/
    del_F                  += ttlik1-dp_1->ttlik;
    /*dp1*/
    dp_1->ttlik             = ttlik1;
    /*base*/
    base->numclass          = numclass;
    base->lastcl            = lastcl;
    free(del_col);
    free(tmp_del);
    free(copy_del);
    free(order);
    return del_F;
    }

double hdp_c_mergetable(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res, int exclude) {
    /*0) variables to be read in from struct */
    int numclass, lastcl, maxcl, numtable, totalnt, numdata;
    int *datass, *datatt, *tablecc, *tablend ;
    int *classnt, *classnd;
    OLink *classqq, *tabless, *ctindex,*wordss,*wordqq;
    HH *hh;
    double ttlik, ttlik1, *cclik, alpha, gamma;
    /* from dp */
    datatt   = dp->datatt;
    tablecc  = dp->tablecc;
    tabless  = dp->tabless;
    wordss   = dp->wordss;
    alpha    = dp->alpha;
    numtable = dp->numtable;
    numdata  = dp->numdata;
    ttlik    = dp->ttlik;
    /* from dp_1 */
    gamma    = dp_1->alpha;
    ttlik1   = dp_1->ttlik;
    totalnt  = dp_1->numtable;
    /* from base */
    classqq  = base->classqq;
    wordqq   = base->wordqq;
    numclass = base->numclass;
    cclik    = base->cclik;
    hh       = base->hh;
    ctindex  = base->ctindex;
    lastcl   = base->lastcl;
    maxcl    = base->maxclass;
    /* temp variables */
    int ii, jj, kk, ss, prenumta, oldcc, tablepo, classpo, tmpcc, tablenc;
    double etasum, del_tt, del_tt1, del_cc, tmplik[2], tmpbest, oldclasslik, newclasslik, oldttlik, oldttlik1, prelik, tmplikk, tmpliktt1, pp, tt1;
    double t_oldclasslik, t_newclasslik, t_oldttlik1, t_newttlik1, del_F=0;
    int t_classpo, t_tmpcc,wnum;
    etasum                   = hh[0].eta[0];
    /*prenumta                 = numtable;*/
    prenumta                 = dp->lastta+1;
    OLink tmp_table;
    tmp_table    = malloc(sizeof(OLNode));
    tmp_table->down =NULL;
    tmp_table->val=0;
    tmp_table->o_val=0;

    double *tmp_gl=malloc(sizeof(double)*prenumta);

    for(ii=0; ii<prenumta; ii++) {
        if(tabless[ii]!=NULL&&tabless[ii]->val!=0) {
            tmp_gl[ii]=gamln(tabless[ii]->val,tabless[ii]->val,1);
            }
        }
    double sum_gl,copy_gl;
    double *del_col=malloc(sizeof(double)*(1+base->numword));
    double *del_col2=malloc(sizeof(double)*(1+base->numword));
    double *del_col3=malloc(sizeof(double)*(1+base->numword));
    double *tmp_del=malloc(sizeof(double)*(1+base->numword));
    double *copy_del=malloc(sizeof(double)*(1+base->numword));
    double *copy_del2=malloc(sizeof(double)*(1+base->numword));

    double newdel_tt=0,newdel_tt2;
    double *table_del=malloc(sizeof(double)*(1+base->numword));
    double *table_copy=malloc(sizeof(double)*(1+base->numword));

    int *order;
    order=malloc(sizeof(int)*(prenumta));
    randperm(prenumta,order);
    int iii;
    for(iii=0; iii<prenumta; iii++) {
        ii=order[iii];
        /*if(res==58&&iii==2){break;}*/
        if(tabless[ii]!=NULL&&tabless[ii]->val!=0 && numtable!=1) {
            oldcc                  = tablecc[ii];
            /* 1) remove table item from model*/
            /*remove from base*/
            if(classqq[oldcc]->val!=tabless[ii]->val) {
                del_cc              = delta_k(classqq[oldcc], tabless[ii],hh[oldcc].eta,  -1,del_col);
                del_tt1             = -log(ctindex[oldcc]->val-1);
                }
            else {
                del_cc              = -cclik[oldcc];
                del_tt1             = -log(gamma);
                numclass           -= 1;
                for(ss=1; ss<=base->numword; ss++) {
                    del_col[ss]=gamln(0,hh[oldcc].eta[ss],4);
                    }
                del_col[0]=gamln(0,hh[0].eta[0],5);
                }
            /*remove from dp_1*/
            totalnt               -= 1;
            del_tt1               += log(totalnt+gamma);
            /*remove from dp*/
            tablenc                = tabless[ii]->val;
            del_tt                 = -tmp_gl[ii]-log(alpha);
            /* 2) search new config*/
            tablepo                = -1;
            classpo                = -1;
            /*copy of table ii*/
            AddCol(tmp_table,tabless[ii],tabless,0,2,NULL);
            prelik                 = del_tt*T+del_tt1+del_cc;
            for(jj=0; jj<prenumta; jj++) {
                if(tabless[jj]!=NULL&&tabless[jj]->val!=0&&ii!=jj) {
                    /* 2.1) merge two table*/
                    t_tmpcc             = tablecc[jj];
                    t_oldttlik1         = 0;
                    t_newttlik1         = 0;
                    t_newclasslik       = 0;
                    /*new table term*/
                    newdel_tt           = delta_k(tabless[jj],tabless[ii],NULL ,2,table_del);
                    /* a little hack: if two tables share the same dish, just merge them*/
                    sum_gl            = gamln(tablenc+tabless[jj]->val,tablenc+tabless[jj]->val,1);
                    /*hack: prefer merge...*/
                    if(t_tmpcc==oldcc&&(newdel_tt+del_tt+sum_gl-tmp_gl[jj])*T+del_tt1>0) {
                        tablepo           = jj;
                        oldttlik          = sum_gl-tmp_gl[jj];
                        oldclasslik       = -del_cc;
                        oldttlik1         = 0;
                        newclasslik       = 0;
                        classpo           = oldcc;
                        tmpcc             = oldcc;
                        copy_gl           = sum_gl;
                        newdel_tt2        = newdel_tt;
                        memcpy(table_copy,table_del,sizeof(double)*(1+base->numword));
                        break;
                        }
                    else {
                        tmplik[1]         = sum_gl-tmp_gl[jj];
                        tmplik[0]         = delta_k(classqq[t_tmpcc], tabless[ii],hh[t_tmpcc].eta , 1,copy_del);
                        }
                    /*2.2) find the best k for the merged table*/
                    /*a)stay in t_tmpcc*/
                    t_oldclasslik        = tmplik[0];
                    t_classpo            = t_tmpcc;
                    /*b) cost of remove table jj from class t_tmpcc*/
                    if(ctindex[t_tmpcc]->val!=1) {
                        pp               = delta_k(classqq[t_tmpcc], tabless[jj],hh[t_tmpcc].eta,-1,del_col2);
                        tt1              = -log(ctindex[t_tmpcc]->val-1);
                        }
                    else {
                        pp               = -cclik[t_tmpcc];
                        tt1              = -log(gamma);
                        }
                    /*c) find the best class*/
                    /*creat the merged table*/
                    AddCol(tmp_table,tabless[jj],tabless,0,2,NULL);
                    /*old dish*/
                    for(kk=0; kk<=lastcl; kk++) {
                        if(kk!=oldcc && kk!=t_tmpcc && classqq[kk]!=NULL&&classqq[kk]->val!=0 && kk!=exclude) {
                            tmplikk         = delta_k(classqq[kk],tmp_table, hh[kk].eta, 1,tmp_del);
                            tmpliktt1       = log(ctindex[kk]->val);
                            if(pp+tt1+tmpliktt1+tmplikk-t_oldclasslik-t_newclasslik-t_oldttlik1-t_newttlik1>-HDP_EPSILON) {
                                t_classpo    = kk;
                                t_oldclasslik= pp;
                                t_newclasslik= tmplikk;
                                t_oldttlik1  = tt1;
                                t_newttlik1  = tmpliktt1;
                                memcpy(copy_del,tmp_del,sizeof(double)*(1+base->numword));
                                }
                            }
                        }
                    /*new dish(dont want it for now...hacky)*/
                    /*  tmplikk=delta_k(base,dp_1,dp,prenumta,-1,1);
                     * if(tmplikk+log(gamma)-t_newclasslik-t_newttlik1>0.00001){
                     * t_classpo         = lastcl+1;
                     * t_newttlik1       = log(gamma);
                     * t_newclasslik     = tmplikk;
                     * }
                     */
                    /* recover the last table */
                    DeleteCol(tmp_table,tabless[jj],0,NULL);

                    /*d) decision*/
                    t_oldttlik1=t_oldttlik1+t_newttlik1;
                    if(t_newclasslik+t_oldclasslik+t_oldttlik1+tmplik[1]*T+prelik+newdel_tt*T>HDP_EPSILON) {
                        tablepo           = jj;
                        classpo           = t_classpo;
                        tmpcc             = t_tmpcc;
                        prelik            = -t_newclasslik-t_oldclasslik-t_oldttlik1-tmplik[1]*T;
                        oldttlik1         = t_oldttlik1;
                        oldclasslik       = t_oldclasslik;
                        newclasslik       = t_newclasslik;
                        oldttlik          = tmplik[1];
                        memcpy(copy_del2,copy_del,sizeof(double)*(1+base->numword));
                        memcpy(del_col3,del_col2,sizeof(double)*(1+base->numword));
                        copy_gl           = sum_gl;
                        newdel_tt2        = newdel_tt;
                        memcpy(table_copy,table_del,sizeof(double)*(1+base->numword));
                        }
                    }

                }
            /* recover the last table(or set 0)*/
            /* 3) decision: */
            if(classpo!=-1) {
                /*new config*/
                del_F               += del_cc+oldclasslik+newclasslik;
                /*dp,dp_1 term calculated in the end*/
                /*update base*/
                /*a) oldcc*/
                tmp_gl[tablepo]      = copy_gl;
                cclik[oldcc]        += del_cc;
                cclik[tmpcc]        += oldclasslik;
                if(classpo!=oldcc) {
                    /*if not a simple merge*/
                    DeleteCol(classqq[oldcc],tabless[ii],1,del_col);
                    }
                wnum=AddColElement(ctindex[oldcc],res,oldcc,-1);

                if(tmpcc!=classpo) {
                    /*b) tmpcc*/
                    wnum=AddColElement(ctindex[tmpcc],res,tmpcc,-1);
                    DeleteCol(classqq[tmpcc],tabless[tablepo],1,del_col3);
                    /*CheckGl(base->classqq,base->lastcl,base->hh.eta[1]);*/
                    numclass         -= (classqq[tmpcc]->val==0);
                    /*c) classpo*/
                    wnum=AddColElement(ctindex[classpo],res,classpo,1);
                    cclik[classpo]  += newclasslik;
                    AddCol(tmp_table,tabless[tablepo],tabless,0,2,NULL);
                    AddCol(classqq[classpo],tmp_table,wordqq,classpo,1,copy_del2);
                    /*hack: really dont want to realloc*/
                    }
                else if(classpo!=oldcc) {
                    AddCol(classqq[classpo],tabless[ii],wordqq,classpo,1,copy_del2);
                    }
                /*update dp*/
                ttlik               += newdel_tt2+del_tt+oldttlik;
                AddCol(tabless[tablepo],tabless[ii],wordss,tablepo,1,table_copy);
                SetZero(tabless[ii],0,NULL);
                tablecc[tablepo]     = classpo;
                numtable            -= 1;
                for(kk=0; kk<numdata; kk++) {
                    if(datatt[kk]==ii) {
                        datatt[kk]=tablepo;
                        }
                    }
                /*update dp1*/
                ttlik1              += del_tt1+oldttlik1;
                /*if new class added*/
                if(classpo>lastcl) {
                    numclass         += 1;
                    lastcl           += 1;
                    if(lastcl+2 >=maxcl)
                        printf("need reinitialize maxclass.....\n");
                    printf("we design the sparse matrix to be array of list for faster i/o\n");
                    printf("sry about the bad design......\n");
                    /*alloc more space
                                        hdp_c_resizeclass(dp_1, base);
                                        classqq       = base->classqq;
                                        cclik         = base->cclik;
                                        ctindex       = base->ctindex;
                                        maxcl         = base->maxclass;
                                        classnd       = base->classnd;
                                        classnt       = dp_1->classnd;*/
                    }
                }
            else {
                /*3.2 restore previous config*/
                /*restore base*/
                if(classqq[oldcc]->val==tabless[ii]->val) {
                    numclass           += 1;
                    }
                totalnt               += 1;
                }
            SetZero(tmp_table,0,NULL);
            /*CheckGl(base->classqq,base->lastcl,base->hh.eta[1]);*/
            }
        }

    /*4. update the non-pointers*/
    del_F                  += T*(ttlik-dp->ttlik)+ttlik1-dp_1->ttlik;
    /*dp*/
    dp->numtable            = numtable;
    dp->ttlik               = ttlik;
    /*    dp->lastta              = prenumta-1;

        for(iii=prenumta-1; iii>=0; iii--) {
            if(tabless[iii]!=NULL&&tabless[iii]->val!=0) {
                dp->lastta=iii;
                break;
                }
            }
    */
    /*dp1*/
    dp_1->ttlik             = ttlik1;
    dp_1->numtable          = totalnt;
    /*base*/
    base->numclass          = numclass;
    base->lastcl            = lastcl;
    DelNode(tmp_table);
    free(tmp_gl);
    free(tmp_del);
    free(del_col);
    free(del_col2);
    free(del_col3);
    free(copy_del);
    free(copy_del2);
    free(order);
    return del_F;
    }

double hdp_c_lmres(BASE_C *base, DP_C *dp_1, DP_C *dp, double T, int res, int exclude,int ww,int init) {
    /* temp variables */
    double tmpF=-1, del_F=0;
    int iter=1;
    OLink *classqq,*wordqq;

    while(fabs(tmpF-del_F)>HDP_EPSILON) {

        tmpF=del_F;
        /*1) local searchdatatt*/
        del_F+=hdp_c_localdatatt(base, dp_1, dp, T, res, ww);
        if(ww==-1) {
            /*2) local searchtablecc*/
            del_F+=hdp_c_localtablecc(base, dp_1, dp, res, exclude);
            /*3) merge table*/
            if(init==1) {
                del_F+=hdp_c_mergetable(base, dp_1, dp, T, res, exclude);
                }
            }
        iter+=1;
        }
    return tmpF;
    }

/*3.2) Decompose Restaurant*/
double hdp_c_deleteres(BASE_C *base, DP_C *dp_1, DP_C *dp, int res, double T) {
    int numtable,lastta;
    int *datass, *tablend, *tablecc;
    int *classnt, *classnd;
    OLink *classqq, *tabless, *ctindex;
    HH *hh;
    double *cclik, gamma, del_F=0;
    /* from dp */
    numtable = dp->numtable;
    lastta   = dp->lastta;
    datass   = dp->datass;
    tabless  = dp->tabless;
    tablecc  = dp->tablecc;
    /* from dp_1 */
    gamma    = dp_1->alpha;
    /* from base */
    classqq  = base->classqq;
    cclik    = base->cclik;
    hh       = base->hh;
    ctindex  = base->ctindex;
    int ii, cc, numclass=0,wnum;
    double ttlik1=0,*del_col;
    del_col=malloc(sizeof(double)*(1+base->numword));

    for (ii=0; ii<lastta+1; ii++) {
        if(tabless[ii]!=NULL&&tabless[ii]->val!=0) {
            cc          = tablecc[ii];
            /*base*/
            wnum=AddColElement(ctindex[cc],res,cc,-1);
            if(classqq[cc]->val!= tabless[ii]->val) {
                del_F      -= cclik[cc];
                //printf("cc: %d,%f,%f,%f\n",cc,hh[cc].eta[0],hh[cc].eta[1],cclik[cc]);/**/
                cclik[cc]  += delta_k(classqq[cc], tabless[ii], hh[cc].eta, -1,del_col);
                //printf("cc: %d,%f,%f,%f\n",cc,hh[cc].eta[0],hh[cc].eta[1],cclik[cc]);/**/
                del_F      += cclik[cc];
                DeleteCol(classqq[cc], tabless[ii],1,del_col);
                /*dp1*/
                ttlik1  -= log(ctindex[cc]->val);
                }
            else {
                del_F      -= cclik[cc];
                cclik[cc]   = 0;
                numclass   +=1;
                SetZero(classqq[cc],1,hh[cc].eta);
                /*dp1*/
                ttlik1  -= log(gamma);
                }
            /*dp*/
            tablecc[ii] = -1;
            SetZero(tabless[ii], 0,NULL);
            }
        /*printf("dell  %d,%d,%f\n",ii,wnum,del_F);*/
        }
    /*dp1*/
    ttlik1      += gamln(dp_1->numtable,gamma+dp_1->numtable,3)-gamln(dp_1->numtable-numtable,gamma+dp_1->numtable-numtable,3);
    del_F       += ttlik1-T*dp->ttlik;
    dp_1->ttlik+= ttlik1;
    dp_1->numtable -= numtable;
    /*dp*/
    dp->ttlik   = 0;
    dp->numtable= 0;

    /*base*/
    base->numclass -=numclass;
    free(del_col);
    return del_F;
    }

double hdp_c_randres(BASE_C *base, DP_C *dp_1, DP_C *dp , int freezecl, int res, double T) {
    /*0) variables to be read in from struct */
    int numclass, maxcl, lastcl, numtable, numdata, totalnt;
    int *datass, *datatt, *tablend, *tablecc;
    int *classnt, *classnd;
    OLink *classqq, *tabless, *ctindex,*wordqq, *wordss;
    HH *hh;
    double ttlik, ttlik1, *cclik, alpha, gamma, del_F=0;
    /* from dp */
    numdata  = dp->numdata;
    numtable = 0;
    ttlik    = 0;
    datass   = dp->datass;
    alpha    = dp->alpha;
    datatt   = dp->datatt;
    tabless  = dp->tabless;
    wordss  = dp->wordss;
    tablecc  = dp->tablecc;
    /* from dp_1 */
    gamma    = dp_1->alpha;
    totalnt  = dp_1->numtable;
    ttlik1   = dp_1->ttlik;
    /* from base */
    classqq  = base->classqq;
    wordqq   = base->wordqq;
    numclass = base->numclass;
    cclik    = base->cclik;
    hh       = base->hh;
    lastcl   = base->lastcl;
    ctindex  = base->ctindex;
    /* temp variables */
    int ii, jj, kk, ss, tt, cc, count, oldcc, prenumcl=0,wnum,wnum2,maxta=0;
    int *pp           = malloc(sizeof(int)*numclass);
    int *pp2          = malloc(sizeof(int)*numclass);
    int *fullrow      = malloc(sizeof(int)*(1+lastcl));
    double tabsum=gamma+totalnt, etasum=hh[0].eta[0];
    double *tmpweight, **weight;
    int *index, *njt;/*for matlab sampling*/

    /*1) Initialize tmp variables*/
    for(ii=0; ii<lastcl+1; ii++) {
        if(classqq[ii]!=NULL&&ii!=freezecl && classqq[ii]->val!=0) {
            pp[prenumcl] = ii;
            prenumcl    += 1;
            }
        }
    njt          = malloc(sizeof(int)*prenumcl);
    weight       = malloc(sizeof(double*)*(1+base->numword));
    tmpweight    = malloc(sizeof(double)*prenumcl);
    index        = malloc(sizeof(int)*prenumcl);/*for matlab sampling*/
    memcpy(pp2, pp, sizeof(int)*numclass);
    /*njt*/
    memset(njt, 0, prenumcl*sizeof(int));
    for(ii=0; ii<prenumcl; ii++) {
        /*sample index*/
        index[ii]=ii;
        }
    /*hacky: only sample customers back to odld dish*/
    /*weight(1,:)->-t-term;*/
    weight[0]         = malloc(sizeof(double)*prenumcl);
    for(ii=0; ii<prenumcl; ii++) {
        weight[0][ii]  = alpha;
        }
    /* weight(2,:)->-k-term;*/
    for(jj=1; jj<1+base->numword; jj++) {
        weight[jj]     = malloc(sizeof(double)*prenumcl);
        ExpandRow(wordqq[jj-1],fullrow,lastcl);
        for(ii=0; ii<prenumcl; ii++) {
            if(pp[ii]!=-1) {
                weight[jj][ii]=ctindex[pp[ii]]->val*(fullrow[pp[ii]]+hh[pp[ii]].eta[jj])/(tabsum*(classqq[pp[ii]]->val+etasum));
                }
            else {
                weight[jj][ii]=0;
                }
            }
        }
    /*2) sampling datatt*/
    OLink p;
    for(ii=0; ii<numdata; ii++) {
        ss            = datass[ii];
        prodvector(prenumcl, weight[0], weight[ss], tmpweight);
        p=wordss[ss-1]->right;
        /*new counting term*/
        while(p!=NULL) {
            if(p->val>0) {
                tmpweight[p->col]/=(p->val+1);
                }
            p=p->right;
            }
        /*tt=matlab_randsample(tmpweight,index,prenumcl);*/
        //if(ss==77){
        //printvector(prenumcl,weight[77]);
        //}
        tt=index[randmult(tmpweight, prenumcl, 1)];
        cc=pp2[tt];
        pp[tt]        = -1;
        wnum=AddElement(classqq[cc],wordqq[ss-1],ss-1,cc,1,1,hh[cc].eta);
        del_F  += log(weight[ss][tt]);
        if(njt[tt]==0) {
            ttlik1    -= log(tabsum)-log(ctindex[cc]->val);
            cclik[cc] += log(weight[ss][tt])+log(tabsum)-log(ctindex[cc]->val);
            wnum2=AddColElement(ctindex[cc],res,cc,1);
            totalnt   += 1;
            tabsum    += 1;
            numtable  += 1;
            for(kk=0; kk<prenumcl; kk++) {
                if(pp[kk]!=-1) {
                    for(jj=1; jj<base->numword+1; jj++) {
                        weight[jj][kk]  *= (tabsum-1)/(tabsum);
                        }
                    }
                }
            for(jj=1; jj<base->numword+1; jj++) {
                weight[jj][tt]     *= (tabsum-1)/(ctindex[cc]->val-1);
                }
            }
        else {
            cclik[cc] += log(weight[ss][tt]);
            }
        datatt[ii]          = tt;
        njt[tt]            += 1;
        if(tabless[tt]==NULL) {
            tabless[tt]=malloc(sizeof(OLNode));
            tabless[tt]->down=NULL;
            tabless[tt]->val=0;
            tabless[tt]->o_val=0;
            }
        wnum2=AddElement(tabless[tt],wordss[ss-1],ss-1,tt,1,0,NULL);
        /*update weight*/
        weight[0][tt]       = njt[tt];
        weight[ss][tt]     *= (wnum+hh[cc].eta[ss])/(wnum+hh[cc].eta[ss]-1);
        for(jj=1; jj<base->numword+1; jj++) {
            weight[jj][tt]     *= (classqq[cc]->val+etasum-1)/(classqq[cc]->val+etasum);
            }
        }
    /*3) update dp config*/
    dp->numtable        = numtable;
    for(ii=0; ii<prenumcl; ii++) {
        if(njt[ii]!=0) {
            /*dp*/
            ttlik        += gamln(njt[ii],njt[ii],1);
            tablecc[ii]= pp2[ii];
            dp->lastta      = ii;
            }
        }
    ttlik              += numtable*log(alpha)-gamln(numdata,numdata+alpha,2)+gamln(0,alpha,2);

    /*new counting term*/
    for(ii=0; ii<base->numword; ii++) {
        if(wordss[ii]!=NULL) {
            ttlik+=gamln_0[wordss[ii]->val+1];
            p=wordss[ii]->right;
            while(p!=NULL) {
                ttlik-=gamln_0[p->val+1];
                p=p->right;
                }
            }
        }

    dp->ttlik           = ttlik;
    del_F              += T*ttlik;
    /*update the non-pointers*/
    /*dp1*/
    dp_1->ttlik         = ttlik1;
    dp_1->numtable      = totalnt;
    /*4) free memory*/
    free(njt);
    free(pp);
    free(pp2);
    free(index);
    free(tmpweight);
    for(ii=0; ii<1+base->numword; ii++) {
        free(weight[ii]);
        }
    free(weight);
    free(fullrow);
    return del_F;
    }

double hdp_c_decompres(BASE_C *base, DP_C *dp_1, double T, int freezecl, int *cltindex, int all,int init) {
    /* temp variables */
    double del_F, tmp_F=0;
    int ress, res, ii, exclude,re[2]= {1,1},*none,cc,dd;
    DP_C *dp;
    DP_C *tmp_dp;
    BASE_C *tmp_base;
    OLink *ccc;
    exclude=(freezecl+1)*all-1;

    if(all==1) {
        /*a little too greedy...*/
        /*
         none = malloc(sizeof(int)*base->numdp);
         none[0]             = base->numdp-1;
         for(ii=0; ii<base->numdp-1; ii++) {
             none[ii+1]=ii;
             }
        */
        /*only reconfigure the untouched restaurants, to see the side-effect*/
        none = malloc(sizeof(int)*(base->numdp-cltindex[0]));
        none[0]             = base->numdp-1-cltindex[0];
        cc=1;
        dd=1;
        for(ii=0; ii<base->numdp-1; ii++) {
            if(dd>cltindex[0]||cltindex[dd]!=ii) {
                none[cc]=ii;
                cc+=1;
                }
            else {
                dd+=1;
                }
            }

        tmp_dp=malloc(sizeof(DP_C)*base->numdp);
        tmp_base=malloc(sizeof(BASE_C));
        /*copy base*/
        CopyBase_C(tmp_base,base);
        /*copy dp*/
        CopyDPVector_C(tmp_dp,dp_1,base->numdp,base->maxclass,base->numword);
        }
    /*double *hao;*/
    int *order=malloc(sizeof(int)*cltindex[0]);
    randperm(cltindex[0],order);
    DP_C *ddp;
    for(ress=0; ress<cltindex[0]; ress++) {
        del_F               = 0;
        res                 = cltindex[order[ress]+1];
        dp                  = dp_1+1+res;
        /*1) Delete restaurant configuration */
        del_F               += hdp_c_deleteres(base, dp_1, dp, res, T);
        //printf("d %d,%f\n",res,del_F);
        /*2) Gibbs sample datatt*/
        del_F               += hdp_c_randres(base, dp_1, dp, exclude, res, T);
        //printf("r %f\n",del_F);
        /*3) local-merge moves*/
        del_F               += hdp_c_lmres(base, dp_1, dp, T, res, exclude,-1,init);
        /*4) accept or reject*/
        re[1]=res;
        if(all==0 && del_F<HDP_EPSILON) {
            /*revert new value to old value*/
            hdp_c_oldval(base, dp_1,re);
            del_F=0;
            base->maxta=(dp->numtable>base->maxta)?(dp->numtable):base->maxta;
            }
        else {
            /*update old value to new value*/
            hdp_c_newval(base, dp_1,re);
            }

        tmp_F+=del_F;
        base->maxta=(base->lastcl>base->maxta)?dp->numtable:base->maxta;
        }

    if(all==1) {
        printf("allllll %f\n",tmp_F);
        tmp_F              += hdp_c_decompres(base, dp_1,  T, freezecl, none, 0,init);/**/
        if(tmp_F<HDP_EPSILON) {
            /*revert new value to old value*/
            DeleteDPVector_C(dp_1,base->numdp,base->maxclass,base->numword);
            DeleteBase_C(base,0);
            memcpy(base,tmp_base,sizeof(BASE_C));
            memcpy(dp_1,tmp_dp,sizeof(DP_C)*base->numdp);
            tmp_F=0;
            }
        else {
            /*delete backup*/
            DeleteDPVector_C(tmp_dp,base->numdp,base->maxclass,base->numword);
            DeleteBase_C(tmp_base,0);
            }
        free(tmp_dp);
        free(tmp_base);
        free(none);
        }
    free(order);
    return tmp_F;
    }
/*3.3)Decompose Word*/
double hdp_c_deleteword(BASE_C *base, DP_C *alldp, int word, int cl, int **wodd, int *ress, double T) {
    int numtable, n_word, pre_word, n_data, totalnt, numdata,lastcl;
    int *datass, *tablend, *tablecc, *datatt;
    int *classnt, *classnd;
    OLink *classqq, *tabless, *ctindex,*wordqq,*wordss;
    HH *hh;
    DP_C *dp;
    double *cclik, gamma, del_F;
    /* from dp_1 */
    gamma    = alldp->alpha;
    totalnt  = alldp->numtable;
    /* from base */
    classqq  = base->classqq;
    cclik    = base->cclik;
    hh       = base->hh;
    lastcl   = base->lastcl;
    ctindex  = base->ctindex;
    wordqq   = base->wordqq;
    int count=SearchRow(wordqq[word-1],cl);
    n_word   =count;
    pre_word = n_word;
    n_data   = classqq[cl]->val;
    int i, ii,iii, cc, numclass=0, del_numta=0, res, len_ress=1, tmp_ta,wnum;
    double del_tt=0, del_tt1=0, del_cc, etasum=hh[0].eta[0], tmplike;
    OLNode *p=ctindex[cl]->down;
    int *fullrow,clnt=ctindex[cl]->val;
    int last = (lastcl>=base->maxta)?(lastcl+1):base->maxta;
    fullrow=malloc(sizeof(int)*last);
    for(i=0; i<clnt; i++) {
        if(count==0||p==NULL) {
            break;
            }
        while(p!=NULL&&p->val==0) {
            p         =p->down;
            }
        res      = p->row;
        dp       = alldp+res+1;
        tmp_ta   = dp->numtable;
        wordss   = dp->wordss;
        if(tmp_ta>1&&wordss[word-1]!=NULL) {
            tabless  = dp->tabless;
            datass  = dp->datass;
            tablecc  = dp->tablecc;
            numdata  = dp->numdata;
            datatt   = dp->datatt;
            ExpandRow(wordss[word-1],fullrow,last-1);
            for(ii=0; ii<tmp_ta; ii++) {
                if(fullrow[ii]!=0&&tablecc[ii]==cl) {
                    ress[len_ress] = res;
                    wodd[len_ress-1][0]= fullrow[ii];
                    cc=1;
                    for(iii=0; iii<numdata; iii++) {
                        if(datatt[iii]==ii&&datass[iii]==word) {
                            datatt[iii]  = -1;
                            wodd[len_ress-1][cc] = iii;
                            cc += 1;
                            }
                        if(cc>wodd[len_ress-1][0]) {
                            break;
                            }
                        }
                    count-=fullrow[ii];
                    n_word        -= fullrow[ii];
                    len_ress      += 1;
                    if(fullrow[ii]!=tabless[ii]->val) {
                        tmplike     = gamln(tabless[ii]->val,tabless[ii]->val,1)-gamln(tabless[ii]->val-fullrow[ii],tabless[ii]->val-fullrow[ii],1);
                        }
                    else {
                        tmplike      = log(dp->alpha)+gamln(fullrow[ii],fullrow[ii],1);
                        wnum=AddColElement(ctindex[cl],res,cl,-1);
                        del_numta    += 1;
                        dp->numtable -=1;
                        }
                    wnum=AddElement(tabless[ii],wordss[word-1],word-1,ii,-fullrow[ii],0,NULL);
                    /*new counting factor*/
                    tmplike-= gamln_0[1+fullrow[ii]];
                    del_tt        -= tmplike;
                    dp->ttlik     -= tmplike;
                    }
                }
            }
        p=p->down;
        }

    if(pre_word!=n_word) {
        /*go on refining*/
        ress[0] = len_ress-1;
        /*update base*/
        if(n_data!=n_word) {
            del_cc = gamln(n_data,n_data+etasum,5)-gamln(n_data-pre_word+n_word,n_data-pre_word+n_word+etasum,5)-gamln(pre_word,pre_word+hh[cl].eta[word],4)+gamln(n_word,n_word+hh[cl].eta[word],4);
            cclik[cl] += del_cc;
            }
        else {
            /*no change of classes*/
            del_cc+= cclik[cl];
            }

        /*update dp1:*/
        if(del_numta!=0) {
            if(ctindex[cl]->val!=0) {
                del_tt1 = gamln(ctindex[cl]->val,ctindex[cl]->val,1)-gamln(ctindex[cl]->val+del_numta,ctindex[cl]->val+del_numta,1)+gamln(totalnt,totalnt+gamma,3)-gamln(totalnt-del_numta,totalnt-del_numta+gamma,3);
                }
            else {
                del_tt1 = -gamln(del_numta,del_numta,1)-log(gamma)+gamln(totalnt,totalnt+gamma,3)-gamln(totalnt-del_numta,totalnt-del_numta+gamma,3);
                base->numclass-=1;
                }

            alldp->ttlik   += del_tt1;
            alldp->numtable    -= del_numta;
            }

        wnum=AddElement(classqq[cl],wordqq[word-1],word-1,cl,-pre_word+n_word,1,hh[cl].eta);
        del_F = T*del_tt+del_tt1+del_cc;
        }
    else {
        del_F = 0;
        ress[0]=0;
        }
    free(fullrow);
    return del_F;
    }

double hdp_c_randword(BASE_C *tmp_base, DP_C *tmp_alldp, int word, int cl, int **wodds, int *ress, double T) {
    DP_C *dp;
    int *classnt, *classnd, *tablecc, *tablend, *datatt;
    int numclass, numtable,lastta;
    double *cclik, etasum, eta, ttlik;
    OLink *classqq, *tabless,*wordqq, *wordss;
    HH *hh=tmp_base->hh;
    etasum   = tmp_base->hh[0].eta[0];
    numclass = tmp_base->numclass;
    classqq  = tmp_base->classqq;
    wordqq   = tmp_base->wordqq;
    cclik    = tmp_base->cclik;
    int i, ii, iii,j, class, count=0, res, cc, dd, tt, ttt, len,wnum;
    int *weight2,*index, *index2, *wodd,*fullrow;
    double *weight, tmplike, del_F, del_tt=0, del_cc=0;

    fullrow  = malloc(sizeof(int)*(1+tmp_base->lastcl));
    ExpandRow(wordqq[word-1],fullrow,tmp_base->lastcl);
    int last = (tmp_base->lastcl>=tmp_base->maxta)?(tmp_base->lastcl+1):tmp_base->maxta;
    weight   = malloc(sizeof(double)*last);
    weight2  = malloc(sizeof(int)*last*last);
    index    = malloc(sizeof(int)*last);
    index2   = malloc(sizeof(int)*last);
    /*the first element of the array is its length*/

    for(i=0; i<ress[0]; i++) {
        res   = ress[i+1];
        dp       = tmp_alldp+res+1;
        numtable = dp->numtable;
        lastta = dp->lastta;
        wordss  = dp->wordss;
        tablecc  = dp->tablecc;
        datatt   = dp->datatt;
        tabless  = dp->tabless;
        ttlik    = dp->ttlik;
        wodd     = wodds[count];

        /*at least 2 more tables left*/
        /*hack: dont want the word form its own table*/
        len=0;
        memset(weight2,0,sizeof(int)*last*last);
        for(ii=0; ii<=lastta; ii++) {
            if(tabless[ii]!=NULL&&tabless[ii]->val!=0&&tablecc[ii]!=cl) {
                class       = tablecc[ii];
                weight[len]  = tabless[ii]->val*(fullrow[class]+hh[class].eta[word])/(classqq[class]->val+etasum);
                /*when alpha large, several tables may have the same dish*/
                weight2[class*last]  +=1;
                weight2[class*last+weight2[class*last]] = len;
                index2[len]  = len;
                index[len]   = ii;
                len         += 1;
                }
            }
        if(len>0) {
            for(ii=0; ii<wodd[0]; ii++) {
                dd              = wodd[ii+1];
                /*in order to check result with matlab*/
                /*ttt             = matlab_randsample(weight,index2,len);*/
                ttt             = index2[randmult(weight, len, 1)];
                tt              = index[ttt];
                cc              = tablecc[tt];
                datatt[dd]      = tt;
                tmplike         = log(tabless[tt]->val);
                del_tt          += tmplike;
                ttlik           += tmplike;
                tmplike        -= log(weight[ttt]);
                cclik[cc]      -= tmplike;
                del_cc         -= tmplike;
                fullrow[cc]     +=1;
                wnum=AddElement(tabless[tt],wordss[word-1],word-1,tt,1,0,NULL);
                del_tt          -= log(wnum);/*new counting factor*/
                ttlik           -= log(wnum);/*new counting factor*/
                wnum=AddElement(classqq[cc],wordqq[word-1],word-1,cc,1,1,hh[cc].eta);
                weight[ttt]     = (tabless[tt]->val*(wnum+hh[cc].eta[word]))/(classqq[cc]->val+etasum);
                if(weight2[cc*last]!=1) {
                    /*extra copy*/
                    tmplike=(wnum+hh[cc].eta[word])/(classqq[cc]->val+etasum);
                    for(j=1; j<=weight2[cc*last]; j++) {
                        weight[weight2[cc*last+j]]     = tabless[index[weight2[cc*last+j]]]->val*tmplike;
                        }
                    }
                }
            }
        else {
            /*simply merge it back to the dish there*/
            for(ii=0; ii<=dp->lastta; ii++) {
                if(tabless[ii]!=NULL&&tabless[ii]->val!=0) {
                    tt=ii;
                    }
                }
            cc             = tablecc[tt];
            tmplike        = -gamln(fullrow[cc],fullrow[cc]+hh[cc].eta[word],4)+gamln(wodd[0]+fullrow[cc],wodd[0]+fullrow[cc]+hh[cc].eta[word],4)+gamln(classqq[cc]->val,classqq[cc]->val+etasum,5)-gamln(wodd[0]+classqq[cc]->val,wodd[0]+classqq[cc]->val+etasum,5);
            cclik[cc]     += tmplike;
            del_cc        += tmplike;
            tmplike        = -gamln(tabless[tt]->val,tabless[tt]->val,1);
            wnum=AddElement(tabless[tt],wordss[word-1],word-1,tt,wodd[0],0,NULL);
            tmplike       += gamln(tabless[tt]->val,tabless[tt]->val,1);
            del_tt        += tmplike+gamln_0[fullrow[cc]+1]-gamln_0[fullrow[cc]+wodd[0]+1];
            ttlik         += tmplike+gamln_0[fullrow[cc]+1]-gamln_0[fullrow[cc]+wodd[0]+1];/*new factor*/
            for(iii=0; iii<wodd[0]; iii++) {
                datatt[wodd[iii+1]] = tt;
                }
            wnum=AddElement(classqq[cc],wordqq[word-1],word-1,cc,wodd[0],1,hh[cc].eta);
            fullrow[cc]+=wodd[0];
            }

        /*update non-pointer*/
        dp->ttlik      = ttlik;
        count          = count+1;
        }
    del_F          = T*del_tt+del_cc;
    free(weight);
    free(weight2);
    free(index);
    free(index2);
    free(fullrow);
    return del_F;
    }

double hdp_c_decompword(BASE_C *base, DP_C *alldp, double T, int cl, int all) {
    /*from hdp*/
    int numtable, clnt;
    OLink *classqq, *ctindex;
    /* from base */
    classqq  = base->classqq;
    ctindex     = base->ctindex;
    clnt     = ctindex[cl]->val;
    DP_C *dp;
    /* temp variables */
    double del_F, tmpF=0;
    int **wodd, *wod, *ress, i, word, ii, exclude;
    exclude  = (cl+1)*all-1;
    /*instead of creating all the time*/
    ress     = malloc(sizeof(int)*(clnt+1));/*instead of passing one more variable*/
    wodd     = malloc(sizeof(int*)*clnt);
    /*find max numdata*/
    OLNode *p=ctindex[cl]->down;
    int maxnumdata=0;
    while(p!=NULL) {
        dp=alldp+p->row+1;
        if(maxnumdata<dp->numdata) {
            maxnumdata=dp->numdata;
            }
        p=p->down;
        }
    for(i=0; i<clnt; i++) {
        wodd[i]= malloc(sizeof(int)*maxnumdata);
        }
    int *order,iii;
    order=malloc(sizeof(int)*base->numword);
    randperm(base->numword,order);
    /*  for (word=1; word<base->numword+1; word++) {*/
    for (iii=0; iii<base->numword; iii++) {
        word=order[iii]+1;
        if(SearchRow(base->wordqq[word-1],cl)!=0) {
            del_F                 = 0;
            /*1) Delete word configuration*/
            del_F                += hdp_c_deleteword( base, alldp, word, cl, wodd, ress, T);
            /*printf("del word %f\n",del_F);*/
            /*2) Gibbs sample datatt*/
            del_F                += hdp_c_randword( base, alldp, word, exclude, wodd, ress, T);
            /*            printf("rand word %f\n",del_F);*/
            /*3) local-merge moves*/
            for(i=0; i<ress[0]; i++) {
                if(i==0||ress[i]!=ress[i+1]) {
                    dp                = alldp+ress[i+1]+1;
                    del_F             += hdp_c_lmres(base, alldp, dp, T, ress[i+1], exclude,word-1,0);
                    base->maxta=(dp->lastta>=base->maxta)?(dp->lastta+1):base->maxta;
                    }
                }

            if(del_F < HDP_EPSILON) {
                /*reject*/
                hdp_c_oldval(base, alldp,ress);
                del_F=0;
                }
            else {
                /*accept*/
                hdp_c_newval(base, alldp,ress);
                }

            tmpF            += del_F;
            }
        }

    for(i=0; i<clnt; i++) {
        free(wodd[i]);
        }
    free(wodd);
    free(ress);
    free(order);
    return tmpF;
    }

/*3.4) Decompose Dish*/
double hdp_c_decompclass(BASE_C *base, DP_C *alldp, double T, int cl, int all,int init) {
    /* temp variables */
    double del_F=0,tmp_F;
    int clnt, *cltindex,count=1;

    /*1) Decompose all the words in the class*/
    del_F          += hdp_c_decompword(base, alldp, T, cl, all);

    /*2) Decompose all the restaurants in the class*/
    OLink *ctindex=base->ctindex,*classqq;
    classqq=base->classqq;
    clnt               = classqq[cl]->val;
    if(clnt!=0) {
        OLNode *p;
        clnt               = ctindex[cl]->val;
        cltindex           = malloc(sizeof(int)*(clnt+1));
        p=ctindex[cl]->down;
        while(p!=NULL) {
            if(p->val>0) {
                cltindex[count]=p->row;
                count+=1;
                }
            p=p->down;
            }
        cltindex[0]        = count-1;
        del_F                += hdp_c_decompres(base, alldp,  T, cl, cltindex, all,init);

        free(cltindex);
        }

    ctindex=base->ctindex;
    clnt               = ctindex[cl]->val;
    if(clnt!=0) {
        del_F          += hdp_c_decompword(base, alldp, T, cl, all);
        }
/*
                 */
    return del_F;

    }

/*4. Merge Dish*/
/*4.1 Merge Tables shared by the Dishes*/
int hdp_c_mergetinside(int *combine,OLink *ctindex,int cl1,int cl2,DP_C *alldp,double *del_tt) {

    int i,j,len=0,count=1,tmp_n1,tmp_n2,where[2];
    int *tablecc,sumtable=alldp->numtable,find;
    double alpha,gamma=alldp->alpha;
    OLink ctindex1=ctindex[cl1];
    OLink ctindex2=ctindex[cl2];
    OLink *tabless;
    DP_C *dp;
    memset(del_tt,0,2*sizeof(double));

    OLNode *p;
    OLNode *q;
    p=ctindex1->down;
    q=ctindex2->down;
    /*remove redundant*/
    while(p->val==0) {
        p=p->down;
        }
    while(q->val==0) {
        q=q->down;
        }
    while(p!=NULL&&q!=NULL) {
        while(p->down!=NULL&&p->row<q->row) {
            p=p->down;
            }
        if(p->row==q->row) {
            combine[count]  = q->row;
            count      += 1;
            dp          = alldp+q->row+1;
            tabless     = dp->tabless;
            tablecc     = dp->tablecc;
            alpha       = dp->alpha;
            find=0;
            for(j=0; j<dp->numtable; j++) {
                if(tablecc[j]==cl1) {
                    tmp_n1=tabless[j]->val;
                    where[find]=j;
                    find++;
                    }
                if(tablecc[j]==cl2) {
                    tmp_n2=tabless[j]->val;
                    where[find]=j;
                    find++;
                    }
                if(find==2) {
                    break;
                    }
                }
            /*dp term: 2nd layer*/
            del_tt[0]    += gamln(tmp_n1+tmp_n2,tmp_n1+tmp_n2,1)-gamln(tmp_n1,tmp_n1,1)-gamln(tmp_n2,tmp_n2,1)-log(alpha);
            /*new counting term*/
            del_tt[0]    += delta_k(tabless[where[0]],tabless[where[1]],del_tt, 3,del_tt);
            }
        q=q->down;
        while(q!=NULL&&q->val==0) {
            q=q->down;
            }
        }
    count--;
    if(count!=0) {
        /*dp term: 1st layer*/
        del_tt[1]+= gamln(sumtable,sumtable+gamma,3)-gamln(sumtable-count,sumtable-count+gamma,3);
        }
    combine[0]=count;
    return count;
    }
double hdp_c_mergeclass(HDP_C* hdp, double T,int mergecl) {
    BASE_C *base    = hdp->base;
    DP_C *dp,*alldp = hdp->dp;
    int lastcl,*tablecc,*o_tablecc,totalnt,*datatt,*tablend;
    double gamma;
    OLink *ctindex,*classqq,*wordqq;
    HH *hh=base->hh;
    double del_tt,*cclik;
    /*base*/
    lastcl        = base->lastcl;
    ctindex       = base->ctindex;
    classqq       = base->classqq;
    wordqq        = base->wordqq;
    cclik         = base->cclik;
    /*dp1*/
    totalnt       = alldp->numtable;
    gamma         = alldp->alpha;
    /*tmp*/
    double tmp_min=0,tmplik[2],del_tt1,new_F=0,mergedp1,mergef,tmp_F;
    int i,j,same_nt=0,tmp_nt,classpo=-1,mergend,mergent,tmp_n1,tmp_n2,tmp_m1,tmp_m2;
    OLink *tabless;
    int *tmp_ctindex,*best_ctindex;
    double *del_col,*copy_col;
    mergend       = classqq[mergecl]->val;
    mergent       = ctindex[mergecl]->val;
    mergef        = -base->cclik[mergecl];
    mergedp1      = -log(gamma)-gamln(mergent,mergent,1);
    /*%1) find the best dish to merge*/

    tmp_ctindex=malloc(sizeof(int)*(base->numdp));
    best_ctindex=malloc(sizeof(int)*(base->numdp));
    del_col=malloc(sizeof(double)*(1+base->numword));
    copy_col=malloc(sizeof(double)*(1+base->numword));
    double *del_col2=malloc(sizeof(double)*(1+base->numword));
    for(i=0; i<=lastcl; i++) {
        if(classqq[i]!=NULL&&classqq[i]->val!=0 && i!=mergecl) {
            /*printf("%d,%d\n",i,classqq[i]->val);*/
            tmp_nt      = hdp_c_mergetinside(tmp_ctindex,ctindex,mergecl,i,alldp,tmplik);
            tmp_F       = delta_k(classqq[i],classqq[mergecl],hh[i].eta,1,del_col);
            tmplik[1]  += gamln(mergent+ctindex[i]->val-tmp_nt,mergent+ctindex[i]->val-tmp_nt,1)-gamln(ctindex[i]->val,ctindex[i]->val,1);
            /*printf("mmee %d,%d,%f,%f,%f,%f\n",i,classpo,tmp_min,tmp_F,T*tmplik[0]+tmplik[1],mergef+mergedp1);*/
            /*printf("merge %d,%d,%f,%f\n",i,classpo,tmp_F+T*tmplik[0]+tmplik[1]+mergef+mergedp1-tmp_min,tmp_F);*/
            if(tmp_F+T*tmplik[0]+tmplik[1]+mergef+mergedp1-tmp_min>0.0001) {
                classpo     = i;
                tmp_min     = tmp_F+mergef+mergedp1+T*tmplik[0]+tmplik[1];
                del_tt1     = tmplik[1];
                memcpy(best_ctindex,tmp_ctindex,sizeof(int)*(tmp_nt+1));
                memcpy(copy_col,del_col,sizeof(double)*(1+base->numword));
                same_nt     = tmp_nt;
                new_F       = tmp_F;
                }
            }
        }

    /*%2) if better, update the table/dish config*/
    if(classpo!=-1) {
        tmp_F=tmp_min;
        /*update base*/
        AddCol(classqq[classpo],classqq[mergecl],wordqq,classpo,1,copy_col);
        /*wait for cleaning up*/
        cclik[classpo]      += new_F;
        cclik[mergecl]       = 0;
        base->numclass      -= 1;
        /*update dp1*/
        alldp->ttlik        += del_tt1+mergedp1;
        alldp->numtable     -= same_nt;

        /*update each dp */
        tmp_nt               = 1;
        OLNode *p;
        int where[2],count;
        p=ctindex[mergecl]->down;
        while(p->val==0) {
            p=p->down;
            }
        while(p!=NULL) {
            dp        = alldp+p->row+1;
            tablecc   = dp->tablecc;
            tabless   = dp->tabless;
            if(same_nt==0||same_nt==tmp_nt-1||p->row!=best_ctindex[tmp_nt]) {
                /*just change tablecc*/
                o_tablecc = dp->o_tablecc;
                for(j=0; j<dp->numtable; j++) {
                    if(tablecc[j]==mergecl) {
                        tablecc[j]=classpo;
                        o_tablecc[j]=classpo;
                        break;
                        }
                    }
                }
            else {
                /*overlapped tables*/
                /*naively suppose that no two tables have the same dish*/
                tmp_nt++;
                count=-1;
                for(j=0; j<dp->numtable; j++) {
                    if(tablecc[j]==classpo) {
                        tmp_n1=j;
                        tmp_m1=dp->tabless[tmp_n1]->val;
                        count++;
                        where[count]=j;
                        }
                    if(tablecc[j]==mergecl) {
                        tmp_n2=j;
                        tmp_m2=dp->tabless[tmp_n2]->val;
                        count++;
                        where[count]=j;
                        }

                    }
                dp->ttlik           += gamln(tmp_m2+tmp_m1,tmp_m2+tmp_m1,1)-gamln(tmp_m2,tmp_m2,1)-gamln(tmp_m1,tmp_m1,1)-log(dp->alpha);
                dp->ttlik           += delta_k(tabless[where[0]],tabless[where[1]],NULL, 2,del_col2);
                AddCol(tabless[tmp_n1],tabless[tmp_n2],dp->wordss,tmp_n1,1,del_col2);
                SetZero(tabless[tmp_n2],0,NULL);
                datatt=dp->datatt;
                for(j=0; j<dp->numdata; j++) {
                    if(datatt[j]==tmp_n2) {
                        datatt[j]=tmp_n1;
                        }
                    }
                dp->numtable-=1;
                }
            p=p->down;
            while(p!=NULL&&p->val==0) {
                p=p->down;
                }
            }
        AddCol(ctindex[classpo],ctindex[mergecl],ctindex,classpo,2,NULL);
        ctindex[classpo]->val    -= same_nt;
        p=ctindex[classpo]->down;
        tmp_nt=1;
        while(p!=NULL&&tmp_nt<=same_nt) {
            if(p->row==best_ctindex[tmp_nt]) {
                p->val-=1;
                tmp_nt+=1;
                }
            p=p->down;
            }

        SetZero(classqq[mergecl],1,hh[mergecl].eta);
        SetZero(ctindex[mergecl],2,NULL);
        hdp_c_newval(base, hdp->dp,best_ctindex);

        }
    else {
        tmp_F=0;
        }

    free(tmp_ctindex);
    free(best_ctindex);
    free(del_col);
    free(del_col2);
    free(copy_col);
    return tmp_F;

    }



double hdp_c_searchlambda(BASE_C *base) {
    int i,j,cc,len;
    HH *hh=base->hh;
    OLink *classqq=base->classqq;
    double sss,tmp,del=0,*cclik=base->cclik;
    int *fullcol=malloc(sizeof(int)*base->numword);
    int *fullcol_c=malloc(sizeof(int)*base->numword);
    double *eta_c=malloc(sizeof(double)*(1+base->numword));
    for (i=0;i<base->numclass;i++){
    //for (i=9;i<10;i++){
        //1) preparation
        len=ExpandColNzero(classqq[i],fullcol_c,base->numword);
        eta_c[0]=ExpandCol(classqq[i],fullcol,base->numword);
        eta_c[len]=hh[0].eta[0]-(base->numword-len)*base->noiselevel;
        //initialization:
        //1) in proportion 
        /*
        sss=eta_c[len];
        for (j=1;j<len;j++){
            eta_c[j]=S_STEP*(int)(sss*fullcol_c[j]/(eta_c[0]*S_STEP));
            eta_c[len]-=eta_c[j];
            printf("....,%d %f,%f\n",j,eta_c[j],eta_c[len]);
        } 
        */
        //in avg
        sss=S_STEP*(int)(eta_c[len]/(len*S_STEP));
        for (j=1;j<len;j++){
            eta_c[j]=sss;
            eta_c[len]-=sss;
        } 
        //printvector(len,eta_c);
        //printf("-------------------------%f,%f\n",eta_c[1],eta_c[len]);
        //2) opt for nonzero lambdas
        SearchLambda(fullcol_c,eta_c,len);
        //printvector(len,eta_c);
        //3) calculate the change
        tmp=0; 
        cc=1;
        for (j=0;j<base->numword;j++){
            if(fullcol[j]==0){
               //set to noise level,contribution to likelihood is 0 anyway
               hh[i].eta[j+1]=base->noiselevel;
            }else{
               if(hh[i].eta[j+1]!=eta_c[cc]){
                   tmp-=(gamln(0,hh[i].eta[j+1]+fullcol[j],4)-gamln(0,hh[i].eta[j+1],4));
                   hh[i].eta[j+1]=eta_c[cc];
                   tmp+=(gamln(0,hh[i].eta[j+1]+fullcol[j],4)-gamln(0,hh[i].eta[j+1],4));
               }
               cc+=1;                
            }
        }        
        cclik[i]+=tmp;
        UpdateGl(classqq[i],hh[i].eta);
        //printf("%d,%f,%f\n",i,cclik[i],tmp);
        del+=tmp;
    }
    memcpy(base->o_cclik,base->cclik,sizeof(double)*base->maxclass);
    free(fullcol);
    free(fullcol_c);
    free(eta_c);
    return del;
    }
#endif
