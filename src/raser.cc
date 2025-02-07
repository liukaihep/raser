// 
// Author: SHI Xin <shixin@ihep.ac.cn> 
// Created [2021-03-31 Sun 15:05] 
// Based on KDetSim : http://kdetsim.org 
// 


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <TROOT.h>
#include <TSystem.h>
#include <TString.h>
#include <TH2D.h> 
#include <TFile.h> 
#include <TCanvas.h> 
#include <TStyle.h> 
#include <Riostream.h>
#include <TTree.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TLegend.h>
#include <TApplication.h> 
#include <TMath.h> 
#include <TLine.h> 
#include <TVector3.h> 

// KDetSim 

// nrutil
// double *dvector(long nl, long nh);
#define NR_END 1 
#define FREE_ARG char *
#define EPS 1.0e-14
#define MAXPOINT 10001



static double e_0 = 1.60217733e-19;          //As
static double perm0 = 8.854187817e-12; // F/m
static double Kboltz = 8.617385e-5; // eV/K

void nrerror(char error_text[])
	/* Numerical Recipes standard error handler */
{
	fprintf(stderr, "Numerical Recipes run-time error...\n");
	fprintf(stderr, "%s\n", error_text);
	fprintf(stderr, "...now exiting to system...\n");
	exit(1);
}


double *dvector(long nl, long nh)
	/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v = (double *)malloc((size_t)((nh - nl + 1 + NR_END) * sizeof(double)));
	if (!v)
		nrerror("allocation failure in dvector()");
	return v - nl + NR_END;
}

void free_dvector(double *v, long nl, long nh)
	/* free a double vector allocated with dvector() */
{
	free((FREE_ARG)(v + nl - NR_END));
}

/***************************************************** 
atimes : How to multiply the vector with the matrices 
Modified in this form by GK 4.10.2012
 ******************************************************/

// #define C1(x,y,z) y3[n]=x+y+z;


double *b,*y6,*y2,*y3,*y4,*y5,*y7,*y8; 


/**********************************************************
snrm: Calculates the norm of vector 
unsigned long n - dimension of the vector
double sx[]     - components
itol            - <=3 real norm , otherwise max component.
 ************************************************************/

double snrm(unsigned long n, double sx[], int itol)
{
	unsigned long i,isamax;
	double ans;

	if (itol <= 3) {
		ans = 0.0;
		for (i=1;i<=n;i++) ans += sx[i]*sx[i];
		return sqrt(ans);
	} else {
		isamax=1;
		for (i=1;i<=n;i++) {
			if (fabs(sx[i]) > fabs(sx[isamax])) isamax=i;
		}
		return fabs(sx[isamax]);
	}
}


/***************************************************
  Solve a very simple system of equations 
  with the diagonal elements to be the only ones 
 ****************************************************/
void asolve(unsigned long n, double b[], double x[], int itrnsp)
{
	//Solve a very simple system of n equations
	//with the diagonal elements to be the only ones!
	unsigned long i;

	for(i=1;i<=n;i++) x[i]=(y3[i] != 0.0 ? b[i]/y3[i] : b[i]);
}

void atimes(unsigned long n,int dim[], double x[],double r[],int itrnsp)
{
	// This is a function used to multuply the vectors with matrices!
	// Used for calculation of the electric and ramo field
	int nx,ny,nz;
	int i,j,k,q=0;
	double C,L,D,O,R,U,I;

	nx=dim[0]; ny=dim[1]; nz=dim[2];

	for(k=1; k<=nz; k++)
		for(j=1; j<=ny; j++)          /*mnozenje po stolpcu*/ 
			for(i=1; i<=nx; i++)           /*mnozenje po vrstici*/ 
			{ 
				q++;
				C=y3[q]*x[q];
				if(q-1>1)       L=y2[q]*x[q-1]; else L=0;
				if(q-nx>1)      D=y6[q]*x[q-nx]; else D=0;
				if((q-nx*ny)>1) I=y7[q]*x[q-ny*nx]; else I=0;

				if(q+1<=n)       R=y4[q]*x[q+1]; else R=0;
				if(q+nx<=n)      U=y5[q]*x[q+nx]; else U=0;
				if((q+nx*ny)<=n) O=y8[q]*x[q+ny*nx]; else O=0;

				r[q]=C+L+D+O+R+U+I;
			}
	if(n!=q) printf("\n Error in matrix solving!");
	return;
}



void linbcg(unsigned long n, int dim[], double b[], double x[], int itol, double tol,
		int itmax, int *iter, double *err)
{
	// The main function for electric field calcualtion
	void asolve(unsigned long n, double b[], double x[], int itrnsp);
	void atimes(unsigned long n, int dim[],double x[], double r[], int itrnsp);
	double snrm(unsigned long n, double sx[], int itol);
	double *dvector(long, long);
	void free_dvector(double *, long, long);
	void nrerror(char error_text[]);
	unsigned long j;
	double ak,akden,bk,bkden,bknum,bnrm,dxnrm,xnrm,zm1nrm,znrm;
	double *p,*pp,*r,*rr,*z,*zz;

	p=dvector(1,n);
	pp=dvector(1,n);
	r=dvector(1,n);
	rr=dvector(1,n);
	z=dvector(1,n);
	zz=dvector(1,n);

	*iter=0;
	atimes(n,dim,x,r,0);
	for (j=1;j<=n;j++) {
		r[j]=b[j]-r[j];
		rr[j]=r[j];
	}
	atimes(n,dim,r,rr,0); // minimal residual invariant
	znrm=1.0;
	if (itol == 1) bnrm=snrm(n,b,itol);
	else if (itol == 2) {
		asolve(n,b,z,0);
		bnrm=snrm(n,z,itol);
	}
	else if (itol == 3 || itol == 4) {
		asolve(n,b,z,0);
		bnrm=snrm(n,z,itol);
		asolve(n,r,z,0);
		znrm=snrm(n,z,itol);
	} else nrerror("illegal itol in linbcg");
	asolve(n,r,z,0);
	while (*iter <= itmax) {
		++(*iter);
		zm1nrm=znrm;
		asolve(n,rr,zz,1);
		for (bknum=0.0,j=1;j<=n;j++) bknum += z[j]*rr[j];
		if (*iter == 1) {
			for (j=1;j<=n;j++) {
				p[j]=z[j];
				pp[j]=zz[j];
			}
		}
		else {
			bk=bknum/bkden;
			for (j=1;j<=n;j++) {
				p[j]=bk*p[j]+z[j];
				pp[j]=bk*pp[j]+zz[j];
			}
		}
		bkden=bknum;
		atimes(n,dim,p,z,0);
		for (akden=0.0,j=1;j<=n;j++) akden += z[j]*pp[j];
		ak=bknum/akden;
		atimes(n,dim,pp,zz,1);
		for (j=1;j<=n;j++) {
			x[j] += ak*p[j];
			r[j] -= ak*z[j];
			rr[j] -= ak*zz[j];
		}
		asolve(n,r,z,0);
		if (itol == 1 || itol == 2) {
			znrm=1.0;
			*err=snrm(n,r,itol)/bnrm;
		} else if (itol == 3 || itol == 4) {
			znrm=snrm(n,z,itol);
			if (fabs(zm1nrm-znrm) > EPS*znrm) {
				dxnrm=fabs(ak)*snrm(n,p,itol);
				*err=znrm/fabs(zm1nrm-znrm)*dxnrm;
			} else {
				*err=znrm/bnrm;
				continue;
			}
			xnrm=snrm(n,x,itol);
			if (*err <= 0.5*xnrm) *err /= xnrm;
			else {
				*err=znrm/bnrm;
				continue;
			}
		}
		if(*iter==itmax) {printf("\n Number of iterations exceeded the max. value \n"); 
			printf("iter=%4d err=%12.6f\n",*iter,*err);}

		if (*err <= tol) break;
	}

	free_dvector(p,1,n);
	free_dvector(pp,1,n);
	free_dvector(r,1,n);
	free_dvector(rr,1,n);
	free_dvector(z,1,n);
	free_dvector(zz,1,n);
}

// KStruct 
class KStruct

{
	public:
		Int_t PCharge;
		Int_t Steps;
		Int_t DStrip;
		Float_t Xlenght;
		Float_t Ylenght;
		Float_t Zlenght;
		Float_t TTime;
		Float_t TCharge;
		Float_t Xtrack[MAXPOINT];
		Float_t Ytrack[MAXPOINT];
		Float_t Ztrack[MAXPOINT];
		Float_t Charge[MAXPOINT];
		Float_t Time[MAXPOINT];
		Float_t Efield[MAXPOINT];
		Float_t MulCar[MAXPOINT];


		KStruct();
		~KStruct(){};
		void Clear();  
};

KStruct::KStruct()
{
	Clear();
}


void KStruct::Clear()
{
	Int_t i = 0;
	for (i = 0; i < MAXPOINT; i++) {
		Xtrack[i] = 0;
		Ytrack[i] = 0;
		Ztrack[i] = 0;
		Charge[i] = 0;
		Time[i] = 0;
		Efield[i] = 0;
		MulCar[i] = 0;
	}
	Ylenght = 0;
	Xlenght = 0;
	TTime = 0;
	Steps = 0;
	DStrip = 0;
	TCharge = 0;
}






// KGeometry 

#include "TH3I.h"
#include "TH2F.h"

// #include "TMath.h"
#include "math.h"

TH2F *KHisProject(void *hisIn,Int_t axis,Int_t Bin1)
{ 
	//Projects any quantity maped to geometry in different views
	Int_t i;
	TH2F *his2D;
	TH3F *his=(TH3F *) hisIn;

	Int_t Nx=his->GetNbinsX();
	Int_t Ny=his->GetNbinsY();
	Int_t Nz=his->GetNbinsZ();

	Double_t *Xbins=new Double_t [Nx+1];
	Double_t *Ybins=new Double_t [Ny+1];;
	Double_t *Zbins=new Double_t [Nz+1];;

	//  printf("%d %d %d\n", Nx,Ny,Nz);
	for(i=0;i<=Nx;i++) 
	{
		Xbins[i]=his->GetXaxis()->GetBinLowEdge(i+1);
		//   printf("x:: %d %f\n",i,Xbins[i]);
	}
	for(i=0;i<=Ny;i++) 
	{
		Ybins[i]=his->GetYaxis()->GetBinLowEdge(i+1);
		//   printf("x:: %d %f\n",i,Ybins[i]);
	}

	for(i=0;i<=Nz;i++) 
	{
		Zbins[i]=his->GetZaxis()->GetBinLowEdge(i+1);
		//    printf("x:: %d %f\n",i,Zbins[i]);
	}

	switch(axis)
	{
		case 1:
			his2D=new TH2F("YZ plane","YZ",Ny,Ybins,Nz,Zbins);
			for(int i=1;i<=Ny;i++)
				for(int j=1;j<=Nz;j++)
					his2D->SetBinContent(i,j,his->GetBinContent(Bin1,i,j));
			his2D->GetXaxis()->SetTitle("y [#mum]");
			his2D->GetYaxis()->SetTitle("z [#mum]");
			break;
		case 2:
			his2D=new TH2F("XZ plane","XZ",Nx,Xbins,Nz,Zbins);
			for(int i=1;i<=Nx;i++)
				for(int j=1;j<=Nz;j++)
					his2D->SetBinContent(i,j,his->GetBinContent(i,Bin1,j));
			his2D->GetXaxis()->SetTitle("x [#mum]");
			his2D->GetYaxis()->SetTitle("z [#mum]");
			break;
		case 3:
			his2D=new TH2F("XY plane","XY",Nx,Xbins,Ny,Ybins);
			for(int i=1;i<=Nx;i++)
				for(int j=1;j<=Ny;j++)
					his2D->SetBinContent(i,j,his->GetBinContent(i,j,Bin1));
			his2D->GetXaxis()->SetTitle("x [#mum]");
			his2D->GetYaxis()->SetTitle("y [#mum]");
			break;
	}
	return his2D;
}


class KGeometry
{
	private:
	public:

		TH3I    *EG;        //electrode geometry
		TH3I    *DM;        //detector material
		Int_t nx;           //x-divisions
		Int_t ny;           //y-divisions
		Int_t nz;           //z-divisions 

		// Constructors of the class
		KGeometry();
		~KGeometry();
		KGeometry(TH3I *x){GetGrid(x,0); };
		KGeometry(TH3I *x, TH3I *y){GetGrid(x,0); GetGrid(x,1);};
		void GetGrid(TH3I *,Short_t =0); 
		void ElRectangle(Float_t *Pos, Float_t *Size, Int_t Wei, Int_t Mat);
		void ElCylinder(Float_t *Pos,Float_t R, Float_t L,Int_t O, Int_t Wei, Int_t Mat); 
		Int_t SetBoundaryConditions();
		TH3F *MapToGeometry(Double_t *, Double_t =1);
		TH3F *GetGeom();
		Float_t GetLowEdge(Int_t);
		Float_t GetUpEdge(Int_t);
		//   ClassDef(KGeometry,1) 
};



// ClassImp(KGeometry)
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// KGeometry                                                            //
//                                                                      //
// Class for description of detector geometry                           //
// The class defines the geometry of the detector.                      //
// It is based upon two TH3S histograms                                 // 
// It contains some functions defined to design the electrodes          //  
//                                                                      //
//////////////////////////////////////////////////////////////////////////

KGeometry::KGeometry()
{
	EG=NULL;
	DM=NULL;
	nx=1;
	ny=1;
	nz=1;
}

KGeometry::~KGeometry()
{
	if(EG!=NULL) delete EG;
	if(DM!=NULL) delete DM;
}

void KGeometry::GetGrid(TH3I *x, Short_t which)
{
	// EG grid
	//bit 1 = 1  -> 1st electrode
	//bit 2 = 2  -> 2nd electrode

	switch(which)
	{
		case 0: 
			if(EG!=NULL) delete EG;
			EG=new TH3I(); x->Copy(*EG);
			nx=EG->GetNbinsX();
			ny=EG->GetNbinsY();
			nz=EG->GetNbinsZ();
			break;
		case 1:
			if(DM!=NULL) delete DM;
			DM=new TH3I(); x->Copy(*DM); 
			break;
	}

	if(DM!=NULL) if(DM->GetNbinsX()!=nx) printf("Warning: dimenssions mismatch - X !\n");
	if(DM!=NULL) if(DM->GetNbinsY()!=ny) printf("Warning: dimenssions mismatch - Y !\n");
	if(DM!=NULL) if(DM->GetNbinsZ()!=nz) printf("Warning: dimenssions mismatch - Z !\n");

}

void KGeometry::ElRectangle(Float_t *Pos, Float_t *Size, Int_t Wei, Int_t Mat)
{
	// Sets Up 
	// Int_t i,j,k,q;
	Int_t i,j,k;

	Int_t xpl,ypl,zpl,xpr,ypr,zpr;
	//Set up left edge of the box
	xpl=EG->GetXaxis()->FindBin(Pos[0]-Size[0]);
	ypl=EG->GetYaxis()->FindBin(Pos[1]-Size[1]);
	zpl=EG->GetZaxis()->FindBin(Pos[2]-Size[2]);
	//Set up rigth edge of the box	   
	xpr=EG->GetXaxis()->FindBin(Pos[0]+Size[0]);
	ypr=EG->GetYaxis()->FindBin(Pos[1]+Size[1]);
	zpr=EG->GetZaxis()->FindBin(Pos[2]+Size[2]);
	//	   printf("(%d %d),(%d %d),(%d %d)\n",xpl,xpr,ypl,ypr,zpl,zpr);
	// Fill the geometry histogram
	for(k=zpl;k<=zpr;k++)	   
		for(j=ypl;j<=ypr;j++)
			for(i=xpl;i<=xpr;i++)
			{
				//	  if(x0+i>1 && x0+i<=nx && y0+i>1 && y0+i<=ny)
				//	  printf("%d %d %d %d\n",i,j,k,Wei);
				if(EG!=NULL) EG->SetBinContent(i,j,k,Wei);
				if(DM!=NULL) DM->SetBinContent(i,j,k,Mat);
			}

}


void KGeometry::ElCylinder(Float_t *Pos,Float_t R, Float_t L,Int_t O, Int_t Wei, Int_t Mat)
{
	// Cylindrical electrode 
	// Float_t *Pos;  - postion of the cone center 
	Float_t Dist,D,x,y,z,Bu,Bd;
	//    Int_t i,j,k,q;
	Int_t i,j,k;
	for(k=1;k<=nz;k++)
		for(j=1;j<=ny;j++)
			for(i=1;i<=nx;i++)
			{
				x=EG->GetXaxis()->GetBinCenter(i);
				y=EG->GetYaxis()->GetBinCenter(j);
				z=EG->GetZaxis()->GetBinCenter(k);

				switch(O)
				{
					case 3:   D=TMath::Sqrt(TMath::Power(x-Pos[0],2)+TMath::Power(y-Pos[1],2))-R; break;
					case 2:   D=TMath::Sqrt(TMath::Power(x-Pos[0],2)+TMath::Power(z-Pos[2],2))-R; break;
					case 1:   D=TMath::Sqrt(TMath::Power(y-Pos[1],2)+TMath::Power(z-Pos[2],2))-R; break;
				}

				if(D<=0)
				{
					switch(O)
					{
						case 3:   Dist=EG->GetZaxis()->GetBinCenter(k); 
							  Bu=Pos[2]+L; Bd=Pos[2]-L; break;
						case 2:   Dist=EG->GetYaxis()->GetBinCenter(j); 
							  Bu=Pos[1]+L; Bd=Pos[1]-L; break;
						case 1:   Dist=EG->GetXaxis()->GetBinCenter(i); 
							  Bu=Pos[0]+L; Bd=Pos[0]-L; break;
					}


					if(Dist<=Bu && Dist>=Bd)
					{
						if(EG!=NULL) EG->SetBinContent(i,j,k,Wei); 
						if(DM!=NULL) DM->SetBinContent(i,j,k,Mat);
					}

				}


			}
}

Int_t KGeometry::SetBoundaryConditions()
{
	Int_t i,j,k,val,cval,nval;
	if(EG==NULL) {printf("Please set the geometry first ! \n"); return -1;}
	nx=EG->GetNbinsX();
	ny=EG->GetNbinsY();
	nz=EG->GetNbinsZ();

	//Bit 1 = 1 -> GND - 0 V bias
	//Bit 2 = 2 -> Voltage (usual bias volage)
	//Bit 15-32 = 32768 -> Additional Votlages 

	//Bits determining boundary conditions:
	//bit 2 = 4  -> down val
	//bit 3 = 8  -> up val 
	//bit 4 = 16 -> left val
	//bit 5 = 32 -> rigth val
	//bit 6 = 64  -> down der
	//bit 7 = 128  -> up der 
	//bit 8 = 256 -> left der
	//bit 9 = 512 -> rigth der
	//the 3D section is separated
	//bit 10= 1024 -> out val
	//bit 11= 2048 -> in val
	//bit 12= 4096 -> out der
	//bit 13= 8192 -> in der
	//bit 14= 16384-> read out node
	for(k=1;k<=nz;k++)
	{
		for(j=1;j<=ny;j++)
		{
			for(i=1;i<=nx;i++)
			{
				cval=EG->GetBinContent(i,j,k);

				if(!(cval&1 || cval&2 || cval>=32768))
				{

					nval=0;
					if(i+1<=nx) 
					{
						val=EG->GetBinContent(i+1,j,k);
						if(val&1 || val&2 || val>=32768) nval|=32;
					}
					else nval|=512;

					if(i-1>0)   
					{
						val=EG->GetBinContent(i-1,j,k);
						if(val&1 || val&2 || val>=32768) nval|=16;
					}
					else nval|=256;

					if(j+1<=ny) 
					{

						val=EG->GetBinContent(i,j+1,k);
						if(val&1 || val&2 || val>=32768) nval|=8;
					}
					else nval|=128;

					if(j-1>0)   
					{

						val=EG->GetBinContent(i,j-1,k); 
						if(val&1 || val&2 || val>=32768)  nval|=4; 
					}
					else nval|=64;


					if(k+1<=nz)
					{
						val=EG->GetBinContent(i,j,k+1);
						if(val&1 || val&2 || val>=32768) nval|=2048;
					}
					else if(nz!=1) nval|=8192;

					if(k-1>0)   
					{
						val=EG->GetBinContent(i,j,k-1);
						if(val&1 || val&2 || val>=32768) nval|=1024;
					}
					else  if(nz!=1) nval|=4096;


					EG->SetBinContent(i,j,k,nval);
					//      	if(k==nz) printf("%d %d %d :: %d \n",i,j,k,nval);
				}

			}
		}
	}

	return 0;
}

TH3F *KGeometry::MapToGeometry(Double_t *x,Double_t Scale)
{
	TH3F *fhis=new TH3F();
	EG->Copy(*fhis);
	fhis->Reset();

	// Map the array of values: E, U, W ... to the geometry.
	int i,j,k,n;
	//   Double_t xb=EG->GetXaxis()->GetBinUpEdge(nx);
	//   Double_t yb=EG->GetYaxis()->GetBinUpEdge(ny);
	//   Double_t zb=EG->GetZaxis()->GetBinUpEdge(nz);
	//   Double_t bx=EG->GetXaxis()->GetBinLowEdge(1);
	//   Double_t by=EG->GetYaxis()->GetBinLowEdge(1);
	//   Double_t bz=EG->GetZaxis()->GetBinLowEdge(1);

	//   TH3F *fhis=new TH3F("Pot3d","Pot3d",nx,bx,xb,ny,by,yb,nz,bz,zb);

	for (k=1;k<=nz;k++)
		for (j=1;j<=ny;j++)
			for(i=1;i<=nx;i++)
			{
				n=(k-1)*nx*ny+(j-1)*nx+i; 
				fhis->SetBinContent(i,j,k,x[n]*Scale);
			}
	return fhis;
}

TH3F *KGeometry::GetGeom()
{
	// Map the array of values: E, U, W ... to the geometry.
	//
	int i,j,k,n,bin,col;
	TH3F *dhis=new TH3F();
	EG->Copy(*dhis);

	for (k=1;k<=nz;k++)
		for (j=1;j<=ny;j++)
			for(i=1;i<=nx;i++)
			{
				bin=dhis->GetBinContent(i,j,k);
				col=0;
				if(bin>=32768) col=1; 
				if(bin==1 || bin==2) col=2;
				if(bin==16385) col=3;		 
				dhis->SetBinContent(i,j,k,col);
			}
	//  dhis->Draw("glbox");
	return dhis;
}


Float_t KGeometry::GetUpEdge(Int_t dir)
{
	Float_t ret=0;
	switch(dir)
	{
		case 0: ret=EG->GetXaxis()->GetBinUpEdge(nx); break;
		case 1: ret=EG->GetYaxis()->GetBinUpEdge(ny); break;
		case 2: ret=EG->GetZaxis()->GetBinUpEdge(nz); break;
		default: printf("Index out of scope!\n"); ret=0; break;
	}
	return ret;
}

Float_t KGeometry::GetLowEdge(Int_t dir)
{
	Float_t ret=0;
	switch(dir)
	{
		case 0: ret=EG->GetXaxis()->GetBinLowEdge(1); break;
		case 1: ret=EG->GetYaxis()->GetBinLowEdge(1); break;
		case 2: ret=EG->GetZaxis()->GetBinLowEdge(1); break;
		default: printf("Index out of scope!\n"); ret=0; break;
	}
	return ret;
}




// KMaterial 


class KMaterial
{
	private:
	public:
		static Int_t Mat;              // Material index
		static Float_t Temperature;    // Temperature
		static Int_t Mobility;         // mobility model for each material
		static Int_t ImpactIonization; // impact ionization model

		//////////////////////////////////////////////////////

		KMaterial() { Mat = 1; } // MobMod=1;}
		~KMaterial(){};
static Float_t Perm(Int_t = 1);
static Int_t MobMod();
// ClassDef(KMaterial, 1)
};

// ClassImp(KMaterial)
Int_t KMaterial::Mat = 1;
Float_t KMaterial::Temperature = 293;
Int_t KMaterial::Mobility = 1;
Float_t KMaterial::Perm(Int_t Material)
{
	Float_t perm;
	switch (Material) {
		case 0:
			perm = 11.7;
			break; //silicon
		case 1:
			perm = 11.7;
			break; //poly silicon
		case 2:
			perm = 3.9;
			break; //silicon oxide 2.648
		case 10:
			perm = 5.7;
			break; //diamond
		case 20:
			perm = 1;
			break; //air
		case 100:
			perm = 1;
			break; //aluminium
		default:
			perm = 1;
			break;
	}
	return perm;
}


Int_t KMaterial::MobMod()
{
	Int_t ret;
	switch (Mat) {
		case 0:
			ret = Mobility;
			break; //if(Mobility==1) ret=1; else ret=0; break; //silicon
		case 1:
			ret = 8;
			break; //poly silicon
		case 2:
			ret = 9;
			break; //silicon oxide 2.648
		case 10:
			ret = 10;
			break; //diamond
	}
	return ret;
}


// KField 

class KField
{
	private:
		Int_t Method;   // Method to calculate the intermediate points
		Int_t dim;
	public:
		TH3F *U;
		TH3F *Ex;
		TH3F *Ey;
		TH3F *Ez;
		TH3F *E;

		KField() {U=NULL; Ex=NULL; Ey=NULL; Ez=NULL;};
		~KField();
		Int_t CalField();
		static Float_t GetFieldPoint(Float_t *, Float_t *);
		TVector3 *CalFieldXYZ(Float_t x, Float_t y, Float_t z);
		void  CalFieldXYZ(Float_t x, Float_t y, Float_t z, Float_t *E);  
		void  CalFieldXYZ(Float_t , Float_t , Float_t , TVector3 *); 
		Float_t CalPotXYZ(Float_t x, Float_t y, Float_t z);
		Double_t DriftVelocity(Float_t E,Float_t Charg, Float_t T, Double_t Neff, Int_t which);
		Double_t Mobility(Float_t E,Float_t T,Float_t Charg,Double_t Neff, Int_t which);
		Float_t KInterpolate2D(TH3F *, Float_t ,Float_t, Int_t=3, Int_t=1);

		//   ClassDef(KField,1) 
};

KField::~KField()
{
	if(U!=NULL)  delete U; 
	if(Ex!=NULL) delete Ex; 
	if(Ey!=NULL) delete Ey; 
	if(Ez!=NULL) delete Ez;
}

Float_t KField::KInterpolate2D(TH3F *his, Float_t x, Float_t y, Int_t dir, Int_t bin)
{
	Int_t EX1,EX2,EY1,EY2;
	Float_t t,u,ret;
	TAxis *ax1,*ax2;
	Float_t v11,v21,v22,v12;

	ret=0;

	switch(dir)
	{
		case 3: ax1=his->GetXaxis(); ax2=his->GetYaxis(); break;
		case 2: ax1=his->GetXaxis(); ax2=his->GetZaxis(); break;
		case 1: ax1=his->GetYaxis(); ax2=his->GetZaxis(); break;
	}

	EX1=ax1->FindBin(x); 
	if(ax1->GetBinCenter(EX1)<=x) {EX2=EX1+1;} else {EX2=EX1; EX1--;}
	EY1=ax2->FindBin(y);
	if(ax2->GetBinCenter(EY1)<=y) {EY2=EY1+1;} else {EY2=EY1; EY1--;}

	if(EY2>ax2->GetNbins()) {u=0; EY2=ax2->GetNbins();} else
		if(EY1<1) {u=0; EY1=1;} else
			u=(y-ax2->GetBinCenter(EY1))/(ax2->GetBinCenter(EY2)-ax2->GetBinCenter(EY1));		

	if(EX2>ax1->GetNbins()) {t=0; EX2=ax1->GetNbins();} else
		if(EX1<1) {t=0; EX1=1;} else  
			t=(x-ax1->GetBinCenter(EX1))/(ax1->GetBinCenter(EX2)-ax1->GetBinCenter(EX1));


	//     printf("Points are:: %d %d %d %d [dir=%d, bin=%d] (t=%f u=%f)\n",EX1,EX2,EY1,EY2, dir, bin, t,u);

	switch(dir)
	{
		case 3: 
			v11=his->GetBinContent(EX1,EY1,bin); v21=his->GetBinContent(EX2,EY1,bin);
			v22=his->GetBinContent(EX2,EY2,bin); v12=his->GetBinContent(EX1,EY2,bin);
			break;
		case 2: 
			v11=his->GetBinContent(EX1,bin,EY1); v21=his->GetBinContent(EX2,bin,EY1);
			v22=his->GetBinContent(EX2,bin,EY2); v12=his->GetBinContent(EX1,bin,EY2);
			break;
		case 1: 
			v11=his->GetBinContent(bin,EX1,EY1); v21=his->GetBinContent(bin,EX2,EY1);
			v22=his->GetBinContent(bin,EX2,EY2); v12=his->GetBinContent(bin,EX1,EY2);
			break;
	}

	ret=(1-t)*(1-u)*v11;
	ret+=t*(1-u)*v21;
	ret+=t*u*v22;
	ret+=(1-t)*u*v12;
	return ret;
}


Int_t KField::CalField()
{
	Float_t X[3],Y[3],EE;
	Int_t q,i,j,k;

	if(U==NULL) 
	{printf("Can not calculate field - no potential array!"); return -1;};

	Int_t nx=U->GetNbinsX();
	Int_t ny=U->GetNbinsY();
	Int_t nz=U->GetNbinsZ();

	if(nz==1) {printf("2D field!\n"); dim=2;} else dim=3;



	Ex=new TH3F(); U->Copy(*Ex); Ex->Reset();

	Ey=new TH3F(); U->Copy(*Ey); Ey->Reset();

	Ez=new TH3F(); U->Copy(*Ez); Ez->Reset();

	E=new TH3F(); U->Copy(*E);  E->Reset();


	for(k=1;k<=nz;k++)
		for(j=1;j<=ny;j++)
			for(i=1;i<=nx;i++)
			{

				// Get X field
				if(i==1 || i==nx) Ex->SetBinContent(i,j,k,0); else 
				{
					for(q=0;q<=2;q++) 
					{
						X[q]=U->GetXaxis()->GetBinCenter(i+q-1);
						Y[q]=U->GetBinContent(i+q-1,j,k);
					}

					Ex->SetBinContent(i,j,k,GetFieldPoint(X,Y));
				}


				// Get Y field
				if(j==1 || j==ny) Ey->SetBinContent(i,j,k,0); else 
				{
					for(q=0;q<=2;q++) 
					{
						X[q]=U->GetYaxis()->GetBinCenter(j+q-1);
						Y[q]=U->GetBinContent(i,j+q-1,k);
					}


					Ey->SetBinContent(i,j,k,GetFieldPoint(X,Y));
				}
				// Get Z field
				if(k==1 || k==nz) Ez->SetBinContent(i,j,k,0); else 
				{
					for(q=0;q<=2;q++) 
					{
						X[q]=U->GetZaxis()->GetBinCenter(k+q-1);
						Y[q]=U->GetBinContent(i,j,k+q-1);
					}
					Ez->SetBinContent(i,j,k,GetFieldPoint(X,Y));
				}

				EE=TMath::Sqrt(TMath::Power(Ex->GetBinContent(i,j,k),2)+TMath::Power(Ey->GetBinContent(i,j,k),2)+TMath::Power(Ez->GetBinContent(i,j,k),2));
				E->SetBinContent(i,j,k,EE);

			}

	return 0;
}

Float_t KField::GetFieldPoint(Float_t *X, Float_t *Y)
{
	Float_t a,b,k12,k23;
	k12=(Y[0]-Y[1])/(X[0]-X[1]);
	k23=(Y[1]-Y[2])/(X[1]-X[2]);
	a=(k23-k12)/(X[2]-X[0]);
	b=k12-a*(X[0]+X[1]);
	return 2*a*X[1]+b;
}


void  KField::CalFieldXYZ(Float_t x, Float_t y, Float_t z, Float_t *E)
{
	if(dim==2)
	{
		E[1]=KInterpolate2D(Ex,x,y);
		E[2]=KInterpolate2D(Ey,x,y);
		E[3]=0;
	}
	else
	{
		E[1]=Ex->Interpolate(x,y,z);
		E[2]=Ey->Interpolate(x,y,z);
		E[3]=Ez->Interpolate(x,y,z);
	}

	E[0]=TMath::Sqrt(E[1]*E[1]+E[2]*E[2]+E[3]*E[3]);
}

TVector3 *KField::CalFieldXYZ(Float_t x, Float_t y, Float_t z)
{
	Float_t E[4];
	CalFieldXYZ(x,y,z,E); 
	TVector3 *vec=new TVector3(E[1],E[2],E[3]);
	return vec;
}

void KField::CalFieldXYZ(Float_t x, Float_t y, Float_t z, TVector3 *vec)
{
	Float_t E[4];
	CalFieldXYZ(x,y,z,E); 
	vec->SetXYZ(E[1],E[2],E[3]);
}

Double_t KField::DriftVelocity(Float_t E,Float_t Charg, Float_t T, Double_t Neff, Int_t which)
{
	E*=1e4;
	return(Mobility(E,T,Charg,Neff,which)*(Double_t)E);
}

Float_t KField::CalPotXYZ(Float_t x, Float_t y, Float_t z)
{
	Float_t ret=0;
	Int_t nx,ny,nz,bx,by,bz;
	if(dim==2) ret=KInterpolate2D(U,x,y);
	else 
	{
		nz=U->GetZaxis()->GetNbins(); 
		ny=U->GetYaxis()->GetNbins(); 
		nx=U->GetXaxis()->GetNbins(); 

		if( z >= U->GetZaxis()->GetBinCenter(nz) ) { bz=U->GetZaxis()->FindBin(nz); ret=KInterpolate2D(U,x,y,3,nz); } else
			if( y >= U->GetYaxis()->GetBinCenter(ny) ) { by=U->GetYaxis()->FindBin(ny); ret=KInterpolate2D(U,x,z,2,ny); } else
				if( x >= U->GetXaxis()->GetBinCenter(nx) ) { bx=U->GetXaxis()->FindBin(nx); ret=KInterpolate2D(U,y,z,1,nx); } else 
					if( z <= U->GetZaxis()->GetBinCenter(1)  ) { ret=KInterpolate2D(U,x,y,3,1);                                 } else
						if( y <= U->GetYaxis()->GetBinCenter(1)  ) { ret=KInterpolate2D(U,x,z,2,1);                                 } else
							if( x <= U->GetXaxis()->GetBinCenter(1)  ) { ret=KInterpolate2D(U,y,z,1,1);                                 } else 
								ret=U->Interpolate(x,y,z);
	}
	return ret;
}

Double_t KField::Mobility(Float_t E,Float_t T,Float_t Charg,Double_t Neff, Int_t which)
{
	Double_t lfm=0,hfm=0;
	Double_t vsatn,vsatp,vsat;
	Double_t betap,betan;
	Double_t alpha;

	switch(which)
	{
		case 0:       
			alpha=0.72*TMath::Power(T/300,0.065);
			if(Charg>0)
			{
				Double_t ulp=460*TMath::Power(T/300,-2.18);
				Double_t uminp=45*TMath::Power(T/300,-0.45);      
				Double_t Crefp=2.23e17*TMath::Power(T/300,3.2);
				betap=1;
				vsatp=9.05e6*TMath::Sqrt(TMath::TanH(312/T));
				lfm=uminp+(ulp-uminp)/(1+TMath::Power(Neff/Crefp,alpha));
				hfm=2*lfm/(1+TMath::Power(1+TMath::Power(2*lfm*E/vsatp,betap),1/betap));
			}
			else
			{
				Double_t uln=1430*TMath::Power(T/300,-2); 
				Double_t uminn=80*TMath::Power(T/300,-0.45);
				Double_t Crefn=1.12e17*TMath::Power(T/300,3.2);
				betan=2;      
				vsatn=1.45e7*TMath::Sqrt(TMath::TanH(155/T));
				lfm=uminn+(uln-uminn)/(1+TMath::Power(Neff/Crefn,alpha));
				hfm=2*lfm/(1+TMath::Power(1+TMath::Power(2*lfm*E/vsatn,betan),1/betan));
			}
			break;
		case 1:
			//printf("%e ",par[0]);
			if(Charg>0)
			{
				lfm=8.54e5*TMath::Power(T,-1.075)*TMath::Exp(1-T/124.);
				vsatp=1.445e7*TMath::Exp(-T/435.9);
				betap=2.49*TMath::Exp(-T/270.3);
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatp,1/betap),betap);
			}
			else
			{
				lfm=2.712e8*TMath::Power(T,-2.133);
				vsatn=1.586e7*TMath::Exp(-T/723.6);
				betan=-8.262e-8*TMath::Power(T,3)+6.817e-5*TMath::Power(T,2)-1.847e-2*T+2.429;
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatn,1/betan),betan);
			}
			break; 
		case 2:   // WF2
			if(Charg>0)
			{
				lfm=480;
				vsatp=9.5e6;
				betap=1;
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatp,1/betap),betap);
			}
			else
			{
				lfm=1350;
				vsatn=1.1e7;
				betan=0.5;
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatn,1/betan),betan);
			}
			break; 
		case 3:  // Klanner Scharf
			Double_t bb,cc,E0;

			if(Charg>0)
			{
				E0=2970*TMath::Power(T/300,5.63);
				bb=9.57e-8*TMath::Power(T/300,-0.155);
				cc=-3.24e-13;
				lfm=457*TMath::Power(T/300,-2.80);
				if(E>E0) hfm=1./(1/lfm+bb*(E-E0)+cc*TMath::Power(E-E0,2)); else hfm=lfm;
			}
			else
			{
				E0=2970*TMath::Power(T/300,5.63);
				lfm=1430*TMath::Power(T/300,-1.99);
				vsatn=1.05e7*TMath::Power(T/300,-0.302); //corrected by suggestions of Scharf
				if(E>E0) hfm=1./(1/lfm+1/vsatn*(E-E0)); else hfm=lfm;
			}
			break;
		case 4:   //Jacoboni
			if (Charg>0)
			{
				lfm = 474 * TMath::Power(T/300., -2.619);
				vsatp = 0.940e7  * TMath::Power(T/300., -0.226);
				betap = 1.181 * TMath::Power(T/300., 0.633 ); // <100> orientation
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatp,betap),1/betap);
			} 
			else 
			{
				lfm = 1440*  TMath::Power(T/300., -2.260);
				vsatn = 1.054e7  *  TMath::Power(T/300., -0.602);
				betan = 0.992 *  TMath::Power(T/300., 0.572); // <100> orientation
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatn,betan),1/betan);
			}
			break;
		case 5:   //Jacoboni
			if (Charg>0)
			{
				lfm = 474 * TMath::Power(T/300., -2.619);
				vsatp = 0.940e7  * TMath::Power(T/300., -0.226);
				betap = 1.181 * TMath::Power(T/300., 0.633 ); // <100> orientation
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatp,betap),1/betap);
			} 
			else 
			{
				lfm = 1440*  TMath::Power(T/300., -2.260);
				vsatn = 1.054e7  *  TMath::Power(T/300., -0.602);
				betan = 0.992 *  TMath::Power(T/300., 0.572); // <100> orientation
				hfm=lfm/TMath::Power(1+TMath::Power(lfm*E/vsatn,betan),1/betan);
			}
			break;


		case 9:
			if(Charg>0) {hfm=0;} else {hfm=0;}
			break;
		case 10: //Diamond parametrization
			if(Charg>0) {lfm=2064; vsat=14.1e6;} else {lfm=1714; vsat=9.6e6;};
			hfm=lfm/(1+(lfm*E)/vsat);
			break;
	}
	return hfm; 
}




// KDetector 

#include "TRandom.h"
#include "TF3.h"

class KDetector : public KGeometry, public KMaterial { 


	private:
		Double_t Deps;
		TRandom *ran;               //random number generator
		Double_t CalErr;               //Error of the solver
		Int_t MaxIter;              //Maximum number of iterations in eq solver
		Short_t Debug;              //Print information of drift calculation etc.

	public:
		Float_t Voltage;  //Voltage
		Float_t Voltage2; //Voltage2 
		TArrayF Voltages; //Array of voltages

		// Definition of space charge 
		TF3     *NeffF;     //effective dopping concentration function
		TH3F    *NeffH;     //effective dopping concentration histogram

		// Weigthing, electric and magnetic field
		KField *Ramo;       // ramo field 
		KField *Real;       // electric field
		Float_t B[3];      // magnetic field

		// Trapping and variables used for multiplication studies
		Float_t taue;      // effective trapping time constants 
		Float_t tauh;      // effective trapping time constants 
		TF3 *TauE;         // Function of TauE(x,y,z);
		TF3 *TauH;         // Function of TauH(x,y,z);

		Int_t BreakDown;     // if break down occurs it goes to 1 otherwise is 0
		Float_t MTresh;      // treshold for taking multiplication into account
		Float_t BDTresh;     // hole multiplication - break down treshold 
		Float_t DiffOffField;// electric field where diffusion is switched off [V/um] !!
		// Drift parameters

		Float_t enp[3];      //entry point for the charge drift
		Float_t exp[3];      //exit point for the cahrge drift
		Int_t diff;          // Diffusion simulation (yes=1, no=0)
		Int_t average;       // Average (over how many events)
		Float_t SStep;       // Simulation step size;
		Float_t MaxDriftLen; // Maximum drift lenght before stopping the drift

		// Output histograms
		TH1F *pos;           // contribution of the holes to the total drift current
		TH1F *neg;           // contribution of the electrons  to the total drift current
		TH1F *sum;	       // total drift current

		// Constructors and destructor
		KDetector();
		~KDetector();
		//_______________________________________________________________________________
		void SetDriftHisto(Float_t x,Int_t=200);
		// start declaration followed by solving Poisson's equation.
		void CalField(Int_t);         
		void Declaration(Int_t);                 // declaration of boundary conditions
		Double_t V(int ,int);                    // defining voltage
		Double_t kappa(int ,int , int , int);    // defining space charge

		void ShowMipIR(Int_t, Int_t=14, Int_t=1);
		void ShowUserIonization(Int_t, Float_t *, Float_t *, Float_t *, Float_t *, Int_t=14, Int_t=1);
		void Drift(Double_t, Double_t, Double_t, Float_t, KStruct *, Double_t = 0);



		//Configuration functions 
		// ClassDef(KDetector,1) 
};

#include "TPolyLine3D.h"
// #include "KDetector.h"
#include "TFile.h"

#define ABS(x) x>0?x:-x
#define PREDZNAK(x) x>0?1:-1

#define C1(x,y,z) y3[n]=x+y+z;

#define L1(x) y2[n]=x;
#define R1(x) y4[n]=x;
#define U1(x) y5[n]=x;
#define D1(x) y6[n]=x;
#define I1(x) y7[n]=x;
#define O1(x) y8[n]=x;

#define L2(x) y2[n]=2*x;
#define R2(x) y4[n]=2*x;
#define U2(x) y5[n]=2*x;
#define D2(x) y6[n]=2*x;
#define I2(x) y7[n]=2*x;
#define O2(x) y8[n]=2*x;

#define C0 y3[n]=1.;
#define U0 y5[n]=0.;
#define D0 y6[n]=0.;
#define R0 y4[n]=0.;
#define L0 y2[n]=0.;
#define I0 y7[n]=0.;
#define O0 y8[n]=0.;

#define  PI  3.1415927
#define EPS 1.0e-14
#define STEP_DET_CUR 25e-9

// ClassImp(KDetector)


// double **a,*b,*y6,*y2,*y3,*y4,*y5,*y7,*y8;


void KDetector::SetDriftHisto(Float_t x, Int_t numbins)
{
	if(x<0 || x>10000e-9) 
		printf("Selected time range out of scope !\n"); 
	else
	{
		if(pos!=NULL) delete pos;
		pos  = new TH1F("charge+","Positive Charge",numbins,0,x);
		if(neg!=NULL) delete neg;
		neg  = new TH1F("charge-","Negative Charge",numbins,0,x); 	
		if(sum!=NULL) delete sum;
		sum  = new TH1F("charge","Total Charge",numbins,0,x); 

		sum->SetXTitle("t [s]");  neg->SetXTitle("t [s]"); pos->SetXTitle("t [s]");
		sum->SetYTitle("I [arb.]");neg->SetYTitle("I [arb.]");pos->SetYTitle("I [arb.]");
		sum->GetYaxis()->SetTitleOffset(1.4); neg->GetYaxis()->SetTitleOffset(1.4); pos->GetYaxis()->SetTitleOffset(1.4);
		pos->SetLineColor(2);
		neg->SetLineColor(4);
		sum->SetLineColor(1);	

	}
}



KDetector::KDetector()
{
	//////////////////////////////////////////////////////////////////////
	// Author: Gregor Kramberger                                        //
	// Default constructor for KDetector class                           //
	// Default = no space charge -> default space charge is function     //
	//////////////////////////////////////////////////////////////////////

	NeffF=new TF3("Profile","x[0]*x[1]*x[2]*0+[0]",0,1000,0,1000,0,1000);
	NeffF->SetParameter(0,0);
	NeffH=NULL; //Neff histogram se to NULL

	//set magnetic field to zero
	for(Int_t i=0;i<3;i++) B[i]=0;

	// setting up default random generator - diffusion
	ran=new TRandom(33);  

	// Calculation parameters
	CalErr=1e-6;
	MaxIter=2000;

	// histograms for storing the drift
	pos=NULL; neg=NULL; sum=NULL;
	SetDriftHisto(25e-9);

	// setting up general variables
	// multiplication 
	TauE=NULL;
	TauH=NULL;
	taue=-1;   //no electron trapping 
	tauh=-1;   //no hole trapping 
	MTresh=-1; //no multiplication 
	BDTresh=-1;
	DiffOffField=8;  // critical field for diffusion to be switched off

	// drift

	Deps=1e-5;       //precision of tracking
	MaxDriftLen=1e9; // maximum driftlenght in [um]

	//MobMod=1;  //Mobility parametrization
	average=1; //average over waveforms
	diff=0;    //diffusion
	SStep=1;   //Step size of simulation
	Temperature=263; //temperature
	BreakDown=0; // no breakdown
	Debug=0;     // bo printing of debug information
	Voltage2=0;
	Ramo=new KField();
	Real=new KField();

}


KDetector::~KDetector()
{
	//Destructor of the detector class

	if(NeffF!=NULL) delete NeffF;
	if(NeffH!=NULL) delete NeffH;
	if(ran!=NULL) delete ran;
	if(pos!=NULL) delete pos;
	if(neg!=NULL) delete neg;
	if(sum!=NULL) delete sum;

}

void KDetector::CalField(Int_t what)
{
	double err;
	int iteracije,i;
	int num=nx*ny*nz,dim[3];
	Double_t *x;
	//booking memory
	b=dvector(1,num); y6=dvector(1,num); y2=dvector(1,num); 
	y3=dvector(1,num); y4=dvector(1,num); y5=dvector(1,num);
	y7=dvector(1,num); y8=dvector(1,num);
	x=dvector(1,num);    
	// Setting up the boundary conditions
	printf("Setting up matrix ... \n");
	Declaration(what);
	printf("Solving matrix ...\n");
	// matrix solving
	for(i=1;i<=num;i++) x[i]=1.;
	dim[0]=nx; dim[1]=ny; dim[2]=nz;
	linbcg(num,dim,b,x,1,CalErr,MaxIter,&iteracije,&err);
	// Calculating the field
	if(!what)
	{
		Real->U=MapToGeometry(x);
		Real->CalField();
	}
	else
	{
		// Scale back to 1 from 1000 when storing the potential
		Ramo->U=MapToGeometry(x,1e-4);

		Ramo->CalField();
	}
	// Freeing the memory
	free_dvector(x, 1,num);   free_dvector(b, 1,num);   free_dvector(y2, 1,num); 
	free_dvector(y3, 1,num);  free_dvector(y4, 1,num);  free_dvector(y5, 1,num); 
	free_dvector(y6, 1,num);  free_dvector(y7, 1,num);  free_dvector(y8, 1,num);
}

void KDetector::Declaration(Int_t dowhat)
{
	// New declaration for a general detector class 
	Int_t i,j,k,val;
	Double_t Rd,Ld,Dd,Ud,Od,Id;
	Double_t PRd,PLd,PDd,PUd,POd,PId;

	Double_t Xr=0,Yr=0,Zr=0;
	Double_t Xc=0,Yc=0,Zc=0;
	Double_t Xl=0,Yl=0,Zl=0;
	Int_t ii,jj,kk;
	Double_t fac;

	long n=0;
	Int_t num=nx*ny*nz;

	for (k=1;k<=nz;k++)
		for (j=1;j<=ny;j++)
			for(i=1;i<=nx;i++)
			{
				n=(k-1)*nx*ny+(j-1)*nx+i; //Get index of the matrix element
				if(j-1<1) jj=1;  else jj=j-1;
				if(i-1<1) ii=1;  else ii=i-1; 
				if(k-1<1) kk=1;  else kk=k-1; 

				/////////// DEFINE STEPS IN X //////////////////////////////////////
				Rd=fabs(EG->GetXaxis()->GetBinCenter(i+1)-EG->GetXaxis()->GetBinCenter(i));
				Ld=fabs(EG->GetXaxis()->GetBinCenter(i)-EG->GetXaxis()->GetBinCenter(i-1));
				if(i+1>nx) Rd=Ld; if(i-1<1) Ld=Rd;

				////////// DEFINE PEMITIVITY IN X - normal surface ////////////////////////////
				PRd=Perm(DM->GetBinContent(i,j,k))+Perm(DM->GetBinContent(i,jj,k));

				if(nz!=1) 
				{
					PRd+=Perm(DM->GetBinContent(i,j,kk))+Perm(DM->GetBinContent(i,jj,kk));
					PRd/=4;
				} else PRd/=2;

				PLd=Perm(DM->GetBinContent(ii,j,k))+Perm(DM->GetBinContent(ii,jj,k));
				if(nz!=1) 
				{
					PLd+=Perm(DM->GetBinContent(ii,j,kk))+Perm(DM->GetBinContent(ii,jj,kk));
					PLd/=4;
				} else PLd/=2;

				/////////// DEFINE STEPS IN Y //////////////////////////////////////

				Ud=fabs(EG->GetYaxis()->GetBinCenter(j+1)-EG->GetYaxis()->GetBinCenter(j));
				Dd=fabs(EG->GetYaxis()->GetBinCenter(j)-EG->GetYaxis()->GetBinCenter(j-1));
				if(j+1>ny) Ud=Dd; if(j-1<1) Dd=Ud;

				////////// DEFINE PEMITIVITY IN Y ////////////////////////////
				PUd=Perm(DM->GetBinContent(i,j,k))  +Perm(DM->GetBinContent(ii,j,k));
				if(nz!=1) 
				{
					PUd+=Perm(DM->GetBinContent(i,j,kk))+Perm(DM->GetBinContent(ii,j,kk));
					PUd/=4;
				} else PUd/=2;

				PDd=Perm(DM->GetBinContent(i,jj,k))+Perm(DM->GetBinContent(ii,jj,k));
				if(nz!=1) 
				{
					PDd+=Perm(DM->GetBinContent(i,jj,kk))+Perm(DM->GetBinContent(ii,jj,kk));
					PDd/=4;
				} else PDd/=2;

				/////////// DEFINE STEPS IN Z //////////////////////////////////////

				Od=fabs(EG->GetZaxis()->GetBinCenter(k+1)-EG->GetZaxis()->GetBinCenter(k));
				Id=fabs(EG->GetZaxis()->GetBinCenter(k)-EG->GetZaxis()->GetBinCenter(k-1));
				if(k+1>nz) Od=Id; 
				if(k-1<1) Id=Od;

				//////////DEFINE PEMITIVITY IN Z ////////////////////////////
				if(nz!=1)
				{
					POd=Perm(DM->GetBinContent(i,jj,k))+Perm(DM->GetBinContent(i,j,k))+
						Perm(DM->GetBinContent(ii,j,k))+Perm(DM->GetBinContent(ii,jj,k));

					PId=Perm(DM->GetBinContent(i,jj,kk))+Perm(DM->GetBinContent(i,j,kk))+
						Perm(DM->GetBinContent(ii,j,kk))+Perm(DM->GetBinContent(ii,jj,kk));

					POd/=4;
					PId/=4;

				}
				///////////

				if(dowhat==1) {PRd=1; PLd=1; PUd=1; PDd=1; POd=1; PId=1;}

				Xr=PRd/(0.5*Rd*(Rd+Ld));
				Xl=PLd/(0.5*Ld*(Rd+Ld));  Xc=-(Xr+Xl);
				Yr=PUd/(0.5*Ud*(Ud+Dd));
				Yl=PDd/(0.5*Dd*(Ud+Dd));  Yc=-(Yr+Yl);

				if(nz!=1)
				{
					Zr=POd/(0.5*Od*(Od+Id));
					Zl=PId/(0.5*Id*(Od+Id));  Zc=-(Zr+Zl);
				} 

				//////////

				b[n]=0.;
				val=EG->GetBinContent(i,j,k);

				if(nz==1) { C1(Xc,Yc,0) I0 O0 } 
				else      { C1(Xc,Yc,Zc) I1(Zl) O1(Zr) }

				R1(Xr) U1(Yr) L1(Xl) D1(Yl) 


					if(val&4)            {D0 b[n]-=V(EG->GetBinContent(i,j-1,k),dowhat)*Yl;}
				if(val&8)            {U0 b[n]-=V(EG->GetBinContent(i,j+1,k),dowhat)*Yr;}
				if(val&16)           {L0 b[n]-=V(EG->GetBinContent(i-1,j,k),dowhat)*Xl;}
				if(val&32)           {R0 b[n]-=V(EG->GetBinContent(i+1,j,k),dowhat)*Xr;}
				if(val&1024)         {I0 b[n]-=V(EG->GetBinContent(i,j,k-1),dowhat)*Zl;}
				if(val&2048)         {O0 b[n]-=V(EG->GetBinContent(i,j,k+1),dowhat)*Zr;}


				if(val&64)           {U2(Yr) D0 if(val&8)    {U0 b[n]-=V(EG->GetBinContent(i,j+1,k),dowhat)*Yr;}}
				if(val&128)          {D2(Yl) U0 if(val&4)    {D0 b[n]-=V(EG->GetBinContent(i,j-1,k),dowhat)*Yl;}}
				if(val&256)          {R2(Xr) L0 if(val&32)   {R0 b[n]-=V(EG->GetBinContent(i+1,j,k),dowhat)*Xr;}}
				if(val&512)          {L2(Xl) R0 if(val&16)   {L0 b[n]-=V(EG->GetBinContent(i-1,j,k),dowhat)*Xl;}}
				if(val&4096)         {O2(Zr) I0 if(val&2048) {O0 b[n]-=V(EG->GetBinContent(i,j,k+1),dowhat)*Zr;}}
				if(val&8192)         {I2(Zl) O0 if(val&1024) {I0 b[n]-=V(EG->GetBinContent(i,j,k-1),dowhat)*Zl;}}


				b[n]-=kappa(i,j,k,dowhat);
				if(val&1 || val&2 || val>=32768)   {U0 D0 L0 R0 C0 O0 I0 b[n]=V(val,dowhat);}   
				//if(j<=2 && i<=2) printf("stevilki: i=%d, j=%d, k=%d X=(%f %f ::%f %f), Y(%f %f :: %f %f), Z(%f %f :: %f %f) y[2,3,4,5,6,7,8]=%f %f %f %f %f %f %f :: b[n]=%f :: %d\n",i,j,k,Xr,Xl,Ld,Rd,Yr,Yl,Dd,Ud,Zr,Zl,Id,Od,y2[n],y3[n],y4[n],y5[n],y6[n],y7[n],y8[n],b[n],Mat);
				//      if(k==nz && (j==2 || j==ny-1)) printf("stevilki: i=%d, j=%d, k=%d X=(%f %f ::%f %f), Y(%f %f :: %f %f), Z(%f %f :: %f %f) y[2,3,4,5,6,7,8]=%f %f %f %f %f %f %f :: b[n]=%f\n",i,j,k,Xr,Xl,Ld,Rd,Yr,Yl,Dd,Ud,Zr,Zl,Id,Od,y2[n],y3[n],y4[n],y5[n],y6[n],y7[n],y8[n],b[n]);
			}

}

double KDetector::V(int val, int dowhat)
{
	double voltage;
	int k=0;
	if(dowhat==0) 
	{
		if(val&1) voltage=0;  
		if(val&2) voltage=Voltage;
		if(val & 32768) 
			//if bit 15 is on - many voltages
			voltage=Voltages[val>>16];
	}
	else
	{
		// numerical calculation converges faster if 1000 is used instead of 1
		// therefore the potential is scaled after calculation to 1
		if(val&16384) voltage=10000; else voltage=0;
	}
	return voltage;
}

Double_t KDetector::kappa(int i,int j, int k,  int dowhat )
{
	//Sets the effective space charge values for given point in the mesh!
	Double_t x,y,z,ret;

	//  if(NeffF!=NULL && NeffH!=NULL) printf("Warning:: Histogram values will be taken for Neff!\n");

	//Position in space
	x=EG->GetXaxis()->GetBinCenter(i);
	y=EG->GetYaxis()->GetBinCenter(j);
	z=EG->GetZaxis()->GetBinCenter(k);

	if(DM!=NULL) KMaterial::Mat=DM->GetBinContent(i,j,k); else KMaterial::Mat=0;

	if (dowhat==0) 
	{
		if(NeffF!=NULL)  // Neff=v enotah [um-3]
			//            ret=(NeffF->Eval(x,y,z)*1e6*e_0)/(KMaterial::Perm()*perm0); /*printf("i=%d,j=%d,y=%e\n",i,j,y);*/
			ret=(NeffF->Eval(x,y,z)*1e6*e_0)/(perm0); /*printf("i=%d,j=%d,y=%e\n",i,j,y);*/

		if(NeffH!=NULL)
			//   ret=(NeffH->GetBinContent(i,j,k)*1e6*e_0)/(KMaterial::Perm()*perm0);
			ret=(NeffH->GetBinContent(i,j,k)*1e6*e_0)/(perm0);

	}

	//if (dowhat==0) if(j>nc) y=(Step*Step*1e-12)/(Si_mue*Ro*perm*perm0); else y=-(Step*Step*1e-12)/(Si_mue*Ro*perm*perm0);
	else 
		ret=0.;
	return ret;
}

void KDetector::ShowMipIR(Int_t div, Int_t color,Int_t how)
{
	Float_t *x=new Float_t [div];
	Float_t *y=new Float_t [div];
	Float_t *z=new Float_t [div];
	Float_t *Q=new Float_t [div];

	for(Int_t i=0;i<div;i++) 
	{
		x[i]=((exp[0]-enp[0])/div)*i+enp[0]+(exp[0]-enp[0])/(2*div);
		y[i]=((exp[1]-enp[1])/div)*i+enp[1]+(exp[1]-enp[1])/(2*div);
		z[i]=((exp[2]-enp[2])/div)*i+enp[2]+(exp[2]-enp[2])/(2*div);
	}
	ShowUserIonization(div, x,y,z,Q,color,how);
}

void KDetector::ShowUserIonization(Int_t div, Float_t *x, Float_t *y, Float_t *z, Float_t *Q, Int_t color,Int_t how)
{
	// The simulation of the drift for the minimum ionizing particles. 
	// A track is devided into Int_ div buckets. Each bucket is drifted in the field. The
	// induced currents for each carrier is calculated as the sum  all buckets. 

	TGraph *gr;
	TPolyLine3D *gr3D;
	Float_t sp[3];
	int i,j;
	KStruct seg;
	TLine *line;
	TH3F *hh;

	// Draw histograms 

	if(EG!=NULL) 
	{ 
		if(nz==1) KHisProject(EG,3,how)->Draw("COL");
		else
		{
			//TH3F *hh=GetGeom();
			hh=GetGeom();
			hh->SetFillColor(color);
			hh->Draw("iso");
		}
	} 

	// Draw drift paths

	for(i=0;i<div;i++) 
	{

		// hole drift
		if(Debug)   printf("Entry Point: %f %f %f \n",x[i],y[i],z[i]);
		Drift(x[i],y[i],z[i],1,&seg);

		if(nz==1)
			gr=new TGraph(seg.Steps,&seg.Xtrack[1],&seg.Ytrack[1]); 
		else
			gr3D=new TPolyLine3D(seg.Steps,&seg.Xtrack[1],&seg.Ytrack[1],&seg.Ztrack[1]); 	

		if(nz==1)
		{
			gr->SetLineColor(2);
			gr->SetLineStyle(1);
			gr->Draw("L");
		}
		else
		{
			gr3D->SetLineStyle(1);  
			gr3D->SetLineColor(2); 
			gr3D->Draw("SAME"); 
		}

		// electron drift

		Drift(x[i],y[i],z[i],-1,&seg);

		if(nz==1)
			gr=new TGraph(seg.Steps,&seg.Xtrack[1],&seg.Ytrack[1]); 
		else
		{
			gr3D=new TPolyLine3D(seg.Steps,&seg.Xtrack[1],&seg.Ytrack[1],&seg.Ztrack[1]); 	
		}

		if(nz==1)
		{
			gr->SetLineColor(4);
			gr->SetLineStyle(3); 
			gr->Draw("L");
		}
		else
		{
			gr3D->SetLineStyle(1);  
			gr3D->SetLineColor(4); 
			gr3D->Draw("SAME"); 
		}

	}
}

void KDetector::Drift(Double_t sx, Double_t sy, Double_t sz, Float_t charg, KStruct *seg, Double_t t0)
{
	//Drift simulation for a point charge (Float_t charg;)
	//starting from ( sx,sy, sz)
	//KStruct *seg is the structure where the  the drift paths, drift times and induced cahrges are stored

	Double_t Stime=0;                       // Step time
	Double_t difx=0,dify=0,difz=0;          // diffusion steps in all directions
	Double_t sigma;                         // sigma of the diffusion step  
	Float_t *xcv,*ycv,*zcv,*time,*charge;   // drift variables to be stored in KStruct
	Double_t cx=0,cy=0,cz=0;                // current position of the charge bucket
	Double_t vel=0;                         // drift velocity
	Double_t vth2=0;                        // thermal velocity sqared
	Double_t tfc;                           // trapped charge fraction   
	Double_t t=0;                           // drift time
	Double_t sumc=0;                        // total induced charge
	Double_t ncx=0,ncy=0,ncz=0;             // next position of the charge bucket   
	Double_t deltacx,deltacy,deltacz;       // drift step due to drift

	Int_t st=0;                             // current step
	Int_t ishit=0;                          // local counter of the step
	TVector3 *EE;                           // Set up electric field vector
	TVector3 *EEN;                          // Set up electric field vector
	TVector3 FF;                            // Combined drift field
	Float_t pathlen=0;                      // pathlength
	Float_t WPot;                           // current ramo potential

	// Inclusion of Magnetic field 28.8.2001 - revised 15.10.2012
	TVector3 BB(B);                           // Create a magnetic field vector
	Float_t muhe=1650;                        // parametrization of the magnetic field 
	Float_t muhh=310;	                  // is based on simply this mobility parameters

	// Start time in the  absolute domain (used for delayed charge generation in multiplication 

	t=t0;

	// Intitialize KStruct class and its members 

	xcv=seg->Xtrack; 
	ycv=seg->Ytrack;  
	zcv=seg->Ztrack; 
	time=seg->Time; 
	charge=seg->Charge; 
	seg->Clear();
	seg->PCharge=(Int_t) charg;

	// start drift

	cx=sx; cy=sy; cz=sz;                   // set current coordinates
	xcv[st]=cx; ycv[st]=cy; zcv[st]=cz;    // put the first point in the KStruct 
	time[st]=t;  charge[st]=0; 

	EE=Real->CalFieldXYZ(cx,cy,cz);         // Get the electric field vector 
	EEN=Real->CalFieldXYZ(cx,cy,cz);       // Get the electric field vector for next step - here the default is the same 12.9.2018
	seg->Efield[st]=EE->Mag();             // Store the magnitude of E field






	//printf("%d %f : (%f %f %f)\n",st,charg,cx,cy,cz);

	do 
	{
		//    printf("Calculate field\n");
		st++;
		if(charg>0)
			FF=(*EE)+muhh*EE->Cross(BB); else 
				FF=(*EE)-muhe*EE->Cross(BB); 
		// "-muhe" stands for the fact that at the same 
		// field the drift direction has changed due to different charge

		//printf("Field : %f %f %f (%f %f %f)  ---- ",FF[0],FF[1],FF[2],(*EE)[0],(*EE)[1],(*EE)[2]);
		if(FF.Mag()!=0)
		{
			deltacy=-SStep*charg*FF.y()/FF.Mag();   // deltay 
			deltacx=-SStep*charg*FF.x()/FF.Mag();   // deltax
			deltacz=-SStep*charg*FF.z()/FF.Mag();   // deltaz
		}
		else { deltacx=0; deltacy=0; deltacz=0; }

		//    printf("Calculate velocity \n");

		if(DM!=NULL)
			KMaterial::Mat=DM->GetBinContent(DM->FindBin(cx,cy,cz)); 
		else KMaterial::Mat=0;

		//      EEN=Real->CalFieldXYZ(cx+deltacx,cy+deltacy,cz+deltacz); // get field & velocity at new location //12.9.2018
		Real->CalFieldXYZ(cx+deltacx,cy+deltacy,cz+deltacz,EEN); // get field & velocity at new location
		vel=Real->DriftVelocity( (EEN->Mag()+EE->Mag())/2,charg,Temperature,TMath::Abs(NeffF->Eval(cx,cy,cz)),MobMod());

		//printf("Calculate vel: %e EEN = %e ::: ",vel, EEN->Mag());
		if(vel==0) {
			deltacx=0; deltacy=0; deltacz=0; 
			difx=0; dify=0; difz=0;
			ishit=9;
		} 
		else 
			if(diff && !(DiffOffField<EEN->Mag() && MTresh>1))  
				// is diffusion ON - if yes then include it
				// if multiplication is ON the diffusion must be switched 
				// off when the field gets large enough
			{
				Stime=SStep*1e-4/vel; // calcualte step time  
				sigma=TMath::Sqrt(2*Kboltz*Real->Mobility(EE->Mag(),Temperature,charg,TMath::Abs(NeffF->Eval(cx,cy,cz)),MobMod())*Temperature*Stime); 
				dify=ran->Gaus(0,sigma)*1e4; 
				difx=ran->Gaus(0,sigma)*1e4;
				if(nz!=1) difz=ran->Gaus(0,sigma)*1e4; else difz=0;
			} else {difx=0; dify=0; difz=0;}

		if((cx+deltacx+difx)>=GetUpEdge(0)) ncx=GetUpEdge(0); else
			if((cx+deltacx+difx)<GetLowEdge(0)) ncx=GetLowEdge(0); else
				ncx=cx+(deltacx+difx);;

		if((cy+deltacy+dify)>=GetUpEdge(1)) ncy=GetUpEdge(1); else
			if((cy+deltacy+dify)<GetLowEdge(1)) ncy=GetLowEdge(1); else
				ncy=cy+(deltacy+dify);

		if((cz+deltacz+difz)>=GetUpEdge(2)) ncz=GetUpEdge(2); else
			if((cz+deltacz+difz)<GetLowEdge(2)) ncz=GetLowEdge(2); else
				ncz=cz+(deltacz+difz);

		if(Debug) printf("%d %f E=%e (%e %e %e): x:%f->%f y:%f->%f z:%f->%f (%f %f %f)(%f %f %f) : Mat=%d :: ",st,charg,EEN->Mag(),EEN->x(),EEN->y(),EEN->z(),cx,ncx,cy,ncy,cz,ncz,deltacx,deltacy,deltacz,dify,dify,difz,KMaterial::Mat);

		charge[st]=charg*(Ramo->CalPotXYZ(ncx,ncy,ncz)-Ramo->CalPotXYZ(cx,cy,cz));
		cx=ncx; cy=ncy; cz=ncz;

		//////////////////// calculate strict trapping, e.g. depending on position ///////////////////////
		if(TauE!=NULL && TauH!=NULL)
		{
			if(charg<0) {
				// vth2=3*Kboltz*Temperature*Clight*Clight/(511e3*EmeC(KMaterial::Mat))*1e4*0;
				tfc=1e4*(TauE->Eval((ncx+cx)/2,(ncy+cy)/2,(ncz+cz)/2)*TMath::Sqrt(vel*vel+vth2));	              
			}
			else
			{
				//   vth2=3*Kboltz*Temperature*Clight*Clight/(511e3*EmhC(KMaterial::Mat))*1e4*0;
				tfc=1e4*(TauH->Eval((ncx+cx)/2,(ncy+cy)/2,(ncz+cz)/2)*TMath::Sqrt(vel*vel+vth2));
			}	    
			if(ran->Rndm()>TMath::Exp(-SStep/tfc)) ishit=12;
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////

		sumc+=charge[st];


		if(vel!=0) 
		{
			t=t+SStep*1e-4/vel; //else t+=Stime;
			pathlen+=SStep;
		}

		xcv[st]=cx;
		ycv[st]=cy;
		zcv[st]=cz;
		time[st]=t;

		//    EE=Real->CalFieldXYZ(cx,cy,cz);     //12.9.2018
		Real->CalFieldXYZ(cx,cy,cz,EE);     //12.9.2018
		seg->Efield[st]=EE->Mag();

		// Checking for termination of the drift //// 

		WPot=Ramo->CalPotXYZ(cx,cy,cz);
		if(WPot>(1-Deps)) ishit=1;
		// if(TMath::Abs(WPot)<Deps) ishit=2;
		if(cx<= GetLowEdge(0)) ishit=3;
		if(cx>=  GetUpEdge(0)) ishit=4;
		if(cy<= GetLowEdge(1)) ishit=5;
		if(cy>= GetUpEdge(1))  ishit=6;
		if(cz<= GetLowEdge(2)) ishit=7;
		if(cz>= GetUpEdge(2))  ishit=8;
		if(pathlen>MaxDriftLen) ishit=11;
		if(st>=MAXPOINT-1) ishit=20;   

		if(Debug) printf("(t=%e, vel=%e, velth=%e) [Ch=%f ChInt=%f TFC=%f] Ishit=%d \n",t,vel,TMath::Sqrt(vth2),charge[st],sumc,tfc,ishit);

	} while (!ishit); // Do until the end of drift


	(*seg).Xlenght=pathlen; (*seg).Ylenght=pathlen; 
	(*seg).TTime=t; (*seg).TCharge=sumc; (*seg).Steps=st;
	delete EE; delete EEN;

	return;
}





// K3D 


#include "TArray.h"
#include "TArrayI.h"
#include "TArrayF.h"
#include "TGraph.h"
#include <stdio.h>
#include <stdlib.h>
//#include "KStruct.h"
#include "TMinuit.h"
// #include "KDetector.h"


class K3D : public KDetector
{
	private:
	public:
		Int_t Col;
		Float_t CellZ;
		Float_t CellX;
		Float_t CellY;

		Float_t *PosD; //[Col]
		Float_t *PosX; //[Col]
		Float_t *PosY; //[Col]
		Float_t *PosR; //[Col]
		Short_t *PosW; //[Col]
		Short_t *PosM; //[Col]

		K3D(Int_t, Float_t = 100, Float_t = 100, Float_t = 105);
		~K3D();
		void SetUpVolume(Float_t, Float_t);
		void SetUpColumn(Int_t, Float_t, Float_t, Float_t, Float_t, Short_t, Short_t);
		void SetUpElectrodes(Int_t = 0);

		// ClassDef(K3D, 1)
};

// ClassImp(K3D)
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// K3D                                                                  //
//                                                                      //
// Description of the 3D detector.                                      //
// The geometry of the                                                  //
// is defined by                                                        //
// dasdasd                                                              //
//////////////////////////////////////////////////////////////////////////

K3D::~K3D()
{
	delete PosD;
	delete PosX;
	delete PosY;
	delete PosR;
	delete PosW;
}

K3D::K3D(Int_t x1, Float_t x, Float_t y, Float_t z)
{
	Col = x1;
	PosD = new Float_t[Col];
	PosX = new Float_t[Col];
	PosY = new Float_t[Col];
	PosR = new Float_t[Col];
	PosW = new Short_t[Col];
	PosM = new Short_t[Col];

	for (Int_t i = 0; i < Col; i++) {
		PosD[i] = 0;
		PosX[i] = 0;
		PosY[i] = 0;
		PosR[i] = 0;
		PosW[i] = 0;
		PosM[i] = 0;
	}

	CellZ = z;
	CellX = x;
	CellY = y;
}

void K3D::SetUpVolume(Float_t St1, Float_t St2)
{
	nx = (int)(CellX / St1);
	ny = (int)(CellY / St1);
	nz = (int)(CellZ / St2);

	EG = new TH3I("EG", "EG", nx, 0, CellX, ny, 0, CellY, nz, 0, CellZ);
	EG->GetXaxis()->SetTitle("x [#mum]");
	EG->GetYaxis()->SetTitle("y [#mum]");
	EG->GetZaxis()->SetTitle("z [#mum]");

	GetGrid(EG, 1);
}

void K3D::SetUpColumn(Int_t n, Float_t posX, Float_t posY, Float_t R, Float_t Depth, Short_t Wei, Short_t Mat)
{
	PosD[n] = Depth;
	PosR[n] = R;
	PosX[n] = posX;
	PosY[n] = posY;
	PosW[n] = Wei;
	PosM[n] = Mat;
}

void K3D::SetUpElectrodes(Int_t back)
{
	Float_t Pos[3], L = 0;
	Int_t i, j, k;
	if (back) {
		for (k = 1; k <= nz; k++)
			for (j = 1; j <= ny; j++)
				for (i = 1; i <= nx; i++)
					if (k == 1)
						EG->SetBinContent(i, j, k, 2);
					else
						EG->SetBinContent(i, j, k, 0);
	}

	for (Int_t i = 0; i < Col; i++) {
		Pos[0] = PosX[i];
		Pos[1] = PosY[i];
		Pos[2] = PosD[i] >= 0 ? Pos[2] = PosD[i] / 2. : (2 * CellZ + PosD[i]) / 2.;
		L = TMath::Abs(PosD[i] / 2.);
		ElCylinder(Pos, PosR[i], L, 3, PosW[i], PosM[i]);
	}
}




void set_root_style(int stat=1110, int grid=0){
	gROOT->Reset();

	gStyle->SetTitleFillColor(0) ; 
	gStyle->SetTitleBorderSize(0); 

	gStyle->SetCanvasBorderMode(0);
	gStyle->SetCanvasColor(0);
	gStyle->SetCanvasDefX(0); 
	gStyle->SetCanvasDefY(0); 
	gStyle->SetFrameBorderMode(0); 
	gStyle->SetFrameBorderSize(1); 
	gStyle->SetFrameFillColor(0); 
	gStyle->SetFrameFillStyle(0); 
	gStyle->SetFrameLineColor(1); 
	gStyle->SetFrameLineStyle(1); 
	gStyle->SetFrameLineWidth(1); 

	// gStyle->SetPadTopMargin(PadTopMargin);  
	gStyle->SetPadLeftMargin(0.10);  
	gStyle->SetPadRightMargin(0.05);  

	gStyle->SetLabelSize(0.03, "XYZ");  
	gStyle->SetTitleSize(0.04, "XYZ");  
	gStyle->SetTitleOffset(1.2, "Y");  

	gStyle->SetPadBorderMode(0);  
	gStyle->SetPadColor(0);  
	gStyle->SetPadTickX(1); 
	gStyle->SetPadTickY(1); 
	gStyle->SetPadGridX(grid); 
	gStyle->SetPadGridY(grid); 

	gStyle->SetOptStat(stat); 
	gStyle->SetStatColor(0); 
	gStyle->SetStatBorderSize(1); 
}


TGraph * get_graph_from_log(TString inputFile, TString& err_msg) {
	std::ifstream in;
	in.open(inputFile); 
	Float_t x, y; 
	Float_t factor_I(1.0);
	if (inputFile.Contains("_uA_") ) {
		factor_I = 1e-6;
		std::cout << "Using current factor: " << factor_I << " for " << inputFile << std::endl;  
	} 
	std::vector<Float_t> voltages; 
	std::vector<Float_t> currents; 

	std::string line;
	int nlines = 0;

	// float i_v150, i_v100;
	double i_v150, i_v100;
	bool pass1(false), pass2(false); 

	while (getline(in, line)) {
		std::istringstream iss(line);
		if ( line.find("#") == 0 ) continue; 
		if (!(iss >> x >> y )) break; 
		if (!in.good()) break;
		voltages.push_back(fabs(x));
		currents.push_back(fabs(y)*factor_I);
		// Pick up values like: -100.043	
		if ( fabs(fabs(x)-150) < 1) i_v150 = fabs(y); 
		if ( fabs(fabs(x)-100) < 1) i_v100 = fabs(y); 
		nlines ++; 
	}

	if (nlines < 1) {
		std::cerr << "No valid data found in : " << inputFile << std::endl;
		return NULL; 
	}

	if ( i_v150 < 2E-6) pass1 = true;
	if ( i_v150/i_v100 < 2 ) pass2 = true;

	if (!pass1) err_msg = Form("I(150V) >= 2uA (%.1e)",i_v150) ;
	if (!pass2) err_msg += Form("I(150V)/I(100V) >= 2 (%.1f)", i_v150/i_v100) ;

	in.close();
	TGraph *gr = new TGraph(nlines, &voltages[0], &currents[0]);
	return gr; 
}

TCanvas* drawIV(std::vector<TString> inputFiles){
	set_root_style();

	TCanvas *c = new TCanvas("c", "IV scan", 800, 800);
	c->SetGrid();

	TMultiGraph *mg = new TMultiGraph();
	TLegend *leg = new TLegend(0.2, 0.6, 0.5, 0.8);
	leg->SetBorderSize(0);
	leg->SetFillColor(0);
	leg->SetFillStyle(0);
	leg->SetNColumns(1);
	leg->SetTextSize(0.02);
	leg->SetTextSizePixels(25);

	for (std::vector<int>:: size_type i = 0; i != inputFiles.size(); i++) {
		TString err_msg = "";  
		TGraph *gr = get_graph_from_log(inputFiles[i], err_msg);
		if (gr == NULL) continue; 
		gr->SetMarkerStyle(20+i);
		gr->SetMarkerSize(0.9);
		int color = i+1;
		if (color >= 5) color ++; // bypass the yellow  
		if (color >= 10) color = color % 10 + 1 ; // reuse the first 9 colors
		gr->SetMarkerColor(color);
		leg->AddEntry(gr, Form("%s %s", inputFiles[i].Data(),
					err_msg.Data()), "p"); 
		mg->Add(gr); 
	}

	mg->Draw("APL"); 
	mg->GetXaxis()->SetTitle("Bias Voltage [V]");
	mg->GetYaxis()->SetTitle("Leakage Current [A]");
	mg->GetYaxis()->SetRangeUser(1e-10, 1e-4); 
	leg->Draw(); 

	c->SetLogy();
	c->Update(); 
	return c;
}


#ifndef __CINT__ 

void print_usage(){
	printf("NAME\n\tdrawIV - draw IV curves from log files\n");
	printf("\nSYNOPSIS\n\tdrawIV [-b] [-h] file1 file2 ...\n");
	printf("\nOPTIONS\n");
	printf("\t%-5s  %-40s\n", "-h", "Print this message");
	printf("\t%-5s  %-40s\n", "-b", "Batch mode, save to pdf file directly");
	printf("\nAUTHOR\n\tXin Shi <Xin.Shi@cern.ch>\n");
}

int main(int argc, char** argv) {

	/*
	if (argc < 2) {
		print_usage() ;  
		return -1; 
	}

	//    K3D *det = new K3D(7, 80, 80, 300);

	bool doBatch(false);
	std::vector<TString> inputFiles(argv+1, argv+argc);
	TString outFile = "test.pdf";

	for (int i = 0; i < argc; i++){
		if (!strcmp(argv[i], "-h")) {
			print_usage();
			break; 
		}

		if (!strcmp(argv[i], "-b")) {
			doBatch = true;
			inputFiles.erase(inputFiles.begin()+i-1);
		}
	}

	if (doBatch) { 
		std::cout << "Run in batch mode ... " << std::endl;
		TCanvas *c = drawIV(inputFiles);
		c->SaveAs(outFile);
		delete c;
		gSystem->Exit(0);
	}
	*/

	//TApplication theApp("App", 0, 0);
	//theApp.SetReturnFromRun(true);
	//drawIV(inputFiles); 

	// Start the Test3D_SiC_One
	gStyle->SetCanvasPreferGL(kTRUE);
	// define a 3D detector with 5 electrodes
	// x=100 , y is 50 and thickness 120
	K3D *det = new K3D(7, 80, 80, 300);
	det->Voltage = 50;  
	// define the drift mesh size and simulation mesh size in microns
	det->SetUpVolume(1, 4);
	// define  columns #, postions, weigthing factor 2=0 , material Al=1
	det->SetUpColumn(0, 40, 15, 4, 280, 2, 1);
	det->SetUpColumn(1, 40, 65, 4, 280, 2, 1);
	det->SetUpColumn(2, 61.65, 27.5, 4, 280, 2, 1);
	det->SetUpColumn(3, 61.65, 52.5, 4, 280, 2, 1);
	det->SetUpColumn(4, 18.35, 27.5, 4, 280, 2, 1);
	det->SetUpColumn(5, 18.35, 52.5, 4, 280, 2, 1);
	det->SetUpColumn(6, 40, 40, 4, -280, 16385, 1);
	det->Temperature = 300;
	det->SetDriftHisto(1.2e-9, 36);
	Float_t Pos[3] = {80, 80, 1};
	Float_t Size[3] = {80, 80, 2};
	det->ElRectangle(Pos, Size, 0, 20);  ///how to use?

	det->SetUpElectrodes();
	det->SetBoundaryConditions();

	//define the space charge
	TF3 *f2 = new TF3("f2", "x[0]*x[1]*x[2]*0+[0]", 0, 3000, 0, 3000, 0, 3000);
	f2->SetParameter(0, 2);
	det->NeffF = f2;

	// calculate weigting field
	// calculate electric field
	det->CalField(0);
	det->CalField(1);
	// set entry points of the track
	det->enp[0] = 25;
	det->enp[1] = 40;
	det->enp[2] = 260;
	det->exp[0] = 25;
	det->exp[1] = 40;
	det->exp[2] = 40;

	// switch on the diffusion
	det->diff = 1;
	// Show mip track
	TCanvas c1;
	c1.cd();
	det->ShowMipIR(150);


	//theApp.Run();
}

#endif

