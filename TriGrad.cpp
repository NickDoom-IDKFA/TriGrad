#include <fstream.h>	//for test only
#include <windows.h>	//for min/max only
#include <math.h>	//for test only

struct triangle
{
	int Id;	//remove if don't need
	int X1, Y1;
	int X2, Y2;
	int X3, Y3;
	float Z1, Z2, Z3;
};

//calculates coefficients for z = a*(X-XTp) + b*(Y-Top) + c (actually, one of them)
float Dz_Dx (int XMd, int Mid, int XBm, int Btm, float ZMd, float ZBm)
{
	float Xa, Xb, Ya, Yb, Za, Zb, r;
	//a * XMd + b * Mid = ZMd
	//a * XBm + b * Btm = ZBm
	//Let's decide which one can give a and which one can give b:
	if (!XMd)	//this can give b, not a
	{
		Xa = XBm;
		Xb = 0; //XMd
		Ya = Btm;
		Yb = Mid;
		Za = ZBm;
		Zb = ZMd;
	} else {	//both XBm==0) (this can give b, not a) and (XBm!=0&&XMd!=0) (both equations are all-purpose)
		Xa = XMd;
		Xb = XBm;
		Ya = Mid;
		Yb = Btm;
		Za = ZMd;
		Zb = ZBm;
	}
	//Now we have a * Xa + b * Ya = Za	
	//            a * Xb + b * Yb = Zb	where Xa != 0, Yb != 0 (assuming triangle has not imploded into a black hole!)
	//unless we have rounding troubles, there is no difference to solve a towards b or b towards a; if we'll do, choose bigger abs value.
	if (!Yb) return 0;	//or maybe imploded?
	r = Ya/Yb;
	Za -= Zb*r;
	Xa -= Xb*r;
	//Ya is zero now, don't even bother calculating
	if (!Xa) return 0;
	return Za/Xa;
//	*a = Za / Xa;
	//Let's solve the last one, b*Yb = Zb - a*Xb
//	Zb -= *a * Xb;
//	*b = Zb / Yb;
}

inline float Linear(int x, int y, int x1, int y1, int x2, int y2, float z1, float z2)
{
	int d1, d2;
	d1=abs(x-x1)+abs(y-y1);
	d2=abs(x-x2)+abs(y-y2);
	return (d1*z2+d2*z1)/(d1+d2);
}

void FillTri (float *Area, int Width, triangle Tri, int ClipL, int ClipU, int ClipR, int ClipD)
{
	int Top, Mid, Btm;	//"Top" = "Lowest value" (display lines order);
	int XTp, XMd, XBm;
	int X,Y;
	float ZTp, ZMd, ZBm;
	signed long Dtb, Dtm, Dmb, L, R, Rc;
	float dz, v;

	if (Tri.Y1<=Tri.Y2 && Tri.Y1<=Tri.Y3)	//fast tree (fully unrolled loops)
	{
		Top=Tri.Y1, XTp=Tri.X1;
		ZTp=Tri.Z1;
		if (Tri.Y2<Tri.Y3)
		{
			Mid=Tri.Y2, XMd=Tri.X2;
			ZMd=Tri.Z2;
			Btm=Tri.Y3, XBm=Tri.X3;
			ZBm=Tri.Z3;
		} else {
			Mid=Tri.Y3, XMd=Tri.X3;
			ZMd=Tri.Z3;
			Btm=Tri.Y2, XBm=Tri.X2;
			ZBm=Tri.Z2;
		}
	} else if (Tri.Y2<=Tri.Y1 && Tri.Y2<=Tri.Y3) {
		Top=Tri.Y2, XTp=Tri.X2;
		ZTp=Tri.Z2;
		if (Tri.Y1<Tri.Y3)
		{
			Mid=Tri.Y1, XMd=Tri.X1;
			ZMd=Tri.Z1;
			Btm=Tri.Y3, XBm=Tri.X3;
			ZBm=Tri.Z3;
		} else {
			Mid=Tri.Y3, XMd=Tri.X3;
			ZMd=Tri.Z3;
			Btm=Tri.Y1, XBm=Tri.X1;
			ZBm=Tri.Z1;
		}
	} else {
		Top=Tri.Y3, XTp=Tri.X3;
		ZTp=Tri.Z3;
		if (Tri.Y1<Tri.Y2)
		{
			Mid=Tri.Y1, XMd=Tri.X1;
			ZMd=Tri.Z1;
			Btm=Tri.Y2, XBm=Tri.X2;
			ZBm=Tri.Z2;
		} else {
			Mid=Tri.Y2, XMd=Tri.X2;
			ZMd=Tri.Z2;
			Btm=Tri.Y1, XBm=Tri.X1;
			ZBm=Tri.Z1;
		}
	}

	if (XTp==XMd&&Top==Mid || XBm==XMd&&Btm==Mid)
	{
		return;
	}
if (Btm==Top) return;//(C)Yureg the Gluk
	Dtb=65536*(XBm-XTp)/(Btm-Top);
	R=0;	//Reusing the variable as "triangle Mid is on the right side of Top-Bottom line" flag;
	if (Mid>Top) if ( ( v = (Dtm=65536*(XMd-XTp)/(Mid-Top))-Dtb ) > 0) R=1;
	if (Btm>Mid) if ( ( v = (Dmb=65536*(XBm-XMd)/(Btm-Mid))-Dtb ) < 0) R=1;

	if (fabs(v)<.0001) return;	//Reusing the variable v as "triangle is actually a line";
	else dz=Dz_Dx (XMd-XTp, Mid-Top, XBm-XTp, Btm-Top, ZMd-ZTp, ZBm-ZTp);

	if (R)
	{
		for (Y=Top,L=(XTp<<16)+32768+Dtb/2,R=(XTp<<16)+32768+Dtm/2; Y<Mid; Y++,L+=Dtb,R+=Dtm)
		 if (Y>=ClipU && Y<ClipD)
		  for (v=Linear(X=max(L>>16,ClipL), Y, XTp, Top, XBm, Btm, ZTp, ZBm), Rc=min(R>>16,ClipR);	X<Rc; X++,v+=dz)	//Watcom shift sign propagation
		   Area[Width*Y+X]=v;	//For RGB, I strongly advise to use 16.16 fixed-point instead of float
		for (                              R=(XMd<<16)+32768+Dmb/2; Y<Btm; Y++,L+=Dtb,R+=Dmb)
		 if (Y>=ClipU && Y<ClipD)
		  for (v=Linear(X=max(L>>16,ClipL), Y, XTp, Top, XBm, Btm, ZTp, ZBm), Rc=min(R>>16,ClipR);	X<Rc; X++,v+=dz)	//Watcom shift sign propagation
		   Area[Width*Y+X]=v;	//w/rounding (...remove 
	} else {
		for (Y=Top,L=(XTp<<16)+32768+Dtm/2,R=(XTp<<16)+32768+Dtb/2; Y<Mid; Y++,L+=Dtm,R+=Dtb)
		 if (Y>=ClipU && Y<ClipD)
		  for (v=Linear(X=max(L>>16,ClipL), Y, XTp, Top, XMd, Mid, ZTp, ZMd), Rc=min(R>>16,ClipR);	X<Rc; X++,v+=dz)	//Watcom shift sign propagation
		   Area[Width*Y+X]=v;
		for (      L=(XMd<<16)+32768+Dmb/2                        ; Y<Btm; Y++,L+=Dmb,R+=Dtb)
		 if (Y>=ClipU && Y<ClipD)
		  for (v=Linear(X=max(L>>16,ClipL), Y, XMd, Mid, XBm, Btm, ZMd, ZBm), Rc=min(R>>16,ClipR);	X<Rc; X++,v+=dz)	//Watcom shift sign propagation
		   Area[Width*Y+X]=v;
	}
}

int IdCompare( const void *op1, const void *op2 )
{
    const triangle *p1 = (const triangle *) op1;
    const triangle *p2 = (const triangle *) op2;
    return( p1->Id - p2->Id );
}

void main (void)
{
	static float tstarea [600][800]={0};
	fstream f;
	int i,j,n;

	f.open ("800x600_Mono_Source.raw", ios::in|ios::binary);
	for (int Y=0; Y<600; Y++) for (int X=0; X<800; X++)
	{
		char cha;
		f.read(&cha,1);
		tstarea[Y][X]=cha;
	}
	f.close();

	triangle test[40][60][2];
	int coords[41][61][2];
	for (j=0; j<41; j++)
	 for (i=0; i<61; i++)
	 {			//don't do both at same time: triangles will overlap. Uncomment one of them!!!
		coords[j][i][0]=50+i*10 + rand()*10/RAND_MAX;
		coords[j][i][1]=50+j*10;// + rand()*10/RAND_MAX;
	 }

	for (j=0; j<40; j++)
	{
		for (i=0; i<60; i++)
		{
			test[j][i][0].X1=coords[j  ][i  ][0];
			test[j][i][0].X2=coords[j  ][i+1][0];
			test[j][i][0].X3=coords[j+1][i  ][0];
			test[j][i][0].Y1=coords[j  ][i  ][1];
			test[j][i][0].Y2=coords[j  ][i+1][1];
			test[j][i][0].Y3=coords[j+1][i  ][1];

			test[j][i][1].X1=coords[j  ][i+1][0];
			test[j][i][1].X2=coords[j+1][i  ][0];
			test[j][i][1].X3=coords[j+1][i+1][0];
			test[j][i][1].Y1=coords[j  ][i+1][1];
			test[j][i][1].Y2=coords[j+1][i  ][1];
			test[j][i][1].Y3=coords[j+1][i+1][1];
		}
	}

	triangle temp;
	for (i=0;i<sizeof(test)/sizeof(triangle);i++)
	{
		test[0][0][i].Z1=tstarea[test[0][0][i].Y1][test[0][0][i].X1];
		test[0][0][i].Z2=tstarea[test[0][0][i].Y2][test[0][0][i].X2];
		test[0][0][i].Z3=tstarea[test[0][0][i].Y3][test[0][0][i].X3];

		test[0][0][i].X1-=400,test[0][0][i].X2-=400,test[0][0][i].X3-=400; 	//testing negative coords.
		test[0][0][i].Y1-=300,test[0][0][i].Y2-=300,test[0][0][i].Y3-=300;

		switch (rand()*6/RAND_MAX)	//testing random drawing order
		{
			case 0:	//3, 2, 1
//cout<<"1 ";
				temp.X1=test[0][0][i].X3, temp.X2=test[0][0][i].X2, temp.X3=test[0][0][i].X1;
				temp.Y1=test[0][0][i].Y3, temp.Y2=test[0][0][i].Y2, temp.Y3=test[0][0][i].Y1;
				temp.Z1=test[0][0][i].Z3, temp.Z2=test[0][0][i].Z2, temp.Z3=test[0][0][i].Z1;
				memcpy (test[0][0]+i, &temp, sizeof(triangle));
			break;
			case 1:	//2, 3, 1
//cout<<"2 ";
				temp.X1=test[0][0][i].X2, temp.X2=test[0][0][i].X3, temp.X3=test[0][0][i].X1;
				temp.Y1=test[0][0][i].Y2, temp.Y2=test[0][0][i].Y3, temp.Y3=test[0][0][i].Y1;
				temp.Z1=test[0][0][i].Z2, temp.Z2=test[0][0][i].Z3, temp.Z3=test[0][0][i].Z1;
				memcpy (test[0][0]+i, &temp, sizeof(triangle));
			break;
			case 2:	//2, 1, 3
//cout<<"3 ";
				temp.X1=test[0][0][i].X2, temp.X2=test[0][0][i].X1, temp.X3=test[0][0][i].X3;
				temp.Y1=test[0][0][i].Y2, temp.Y2=test[0][0][i].Y1, temp.Y3=test[0][0][i].Y3;
				temp.Z1=test[0][0][i].Z2, temp.Z2=test[0][0][i].Z1, temp.Z3=test[0][0][i].Z3;
				memcpy (test[0][0]+i, &temp, sizeof(triangle));
			break;
			case 3:	//3, 1, 2
//cout<<"4 ";
				temp.X1=test[0][0][i].X3, temp.X2=test[0][0][i].X1, temp.X3=test[0][0][i].X2;
				temp.Y1=test[0][0][i].Y3, temp.Y2=test[0][0][i].Y1, temp.Y3=test[0][0][i].Y2;
				temp.Z1=test[0][0][i].Z3, temp.Z2=test[0][0][i].Z1, temp.Z3=test[0][0][i].Z2;
				memcpy (test[0][0]+i, &temp, sizeof(triangle));
			break;
			case 4:	//1, 3, 2
//cout<<"5 ";
				temp.X1=test[0][0][i].X1, temp.X2=test[0][0][i].X3, temp.X3=test[0][0][i].X2;
				temp.Y1=test[0][0][i].Y1, temp.Y2=test[0][0][i].Y3, temp.Y3=test[0][0][i].Y2;
				temp.Z1=test[0][0][i].Z1, temp.Z2=test[0][0][i].Z3, temp.Z3=test[0][0][i].Z2;
				memcpy (test[0][0]+i, &temp, sizeof(triangle));
			break;
			case 5:	//1, 2, 3
//cout<<"6 ";
			break;
		}

		test[0][0][i].Id=rand();
	}

	qsort ((void*)test[0][0],sizeof(test)/sizeof(triangle),sizeof(triangle),IdCompare);

	memset (tstarea, 0, sizeof (tstarea));
	for (i=0; i<sizeof(test)/sizeof(triangle); i++)
	{
		FillTri (tstarea[300]+400, 800, test[0][0][i],  -400,  -300, 400, 300);
	}

	f.open ("800x600_Mono.raw", ios::out|ios::binary);
	for (int Y=0; Y<600; Y++) for (int X=0; X<800; X++)
	{
		char cha=tstarea[Y][X]+.5;
		f.write(&cha,1);
	}
	f.close();

cin>>i;
}