// prttools - useful functions for hld*, prt*
// original author: Roman Dzhygadlo - GSI Darmstadt 

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TSpline.h"
#include "TF1.h"
#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TVector3.h"
#include "TMath.h"
#include "TChain.h"
#include "TGaxis.h"
#include "TColor.h"
#include "TString.h"
#include "TArrayD.h"
#include "TSpectrum.h"
#include "TSpectrum2.h"
#include "Math/Minimizer.h"
#include "Math/Factory.h"
#include "Math/Functor.h"
#include "TRandom2.h"
#include "TError.h"
#include "TPaveStats.h"
#include "TObjString.h"
#include "TApplication.h"
#include <TLegend.h>
#include <TAxis.h>
#include <TPaletteAxis.h>
#include <TRandom.h>
#include <TCutG.h>
#include <TKey.h>

#include <iostream>
#include <fstream>
#include <sstream>


#if defined(prt__sim) || defined(prt__beam)

#if defined(prt__beam)
#include "datainfo.C"
#endif

class PrtEvent;
class PrtHit;
PrtEvent* prt_event = 0;
DataInfo prt_data_info;
#endif 

const Int_t prt_nmcp = 12;
const Int_t prt_npix = 64;
const Int_t prt_ntdc = 32;
const Int_t prt_maxdircch(prt_nmcp*prt_npix);
const Int_t prt_maxch = prt_ntdc*48;
const Int_t prt_maxnametdc=10000;

TRandom  prt_rand;
TChain*  prt_ch = 0;
Int_t    prt_entries(0),prt_theta(0),prt_particle(0),prt_geometry(2017),prt_beamx(0),prt_beamz(0);
Double_t prt_test1(0),prt_test2(0),prt_mom(0),prt_phi(0);
TString  prt_savepath(""),prt_info("");
TH2F*    prt_hdigi[prt_nmcp];
TPad*    prt_hpads[prt_nmcp], *prt_hpglobal;
TCanvas* prt_cdigi;
TSpectrum *prt_spect = new TSpectrum(2);

Int_t map_tdc[prt_maxnametdc];
Int_t map_mpc[prt_maxch/64][prt_npix];
Int_t map_mcp[prt_maxch];
Int_t map_pix[prt_maxch];
Int_t map_row[prt_maxch];
Int_t map_col[prt_maxch];


Int_t prt_pid(0), prt_pdg[]={11,13,211,321,2212};
Double_t prt_mass[] = {0.000511,0.1056584,0.139570,0.49368,0.9382723};
TString  prt_name[]={"e","muon","pion","kaon","proton"};
Int_t    prt_color[]={1,1,4,7,2};
Double_t prt_particleArray[3000];

// const Int_t prt_ntdc=16;
// TString prt_tdcsid[prt_ntdc] ={"10","11","12","13",
// 			 "20","21","22","23",
// 			 "780","781","782","783",
// 			 "840","841","842","843"
// };

// const Int_t prt_ntdc=41; //may2015
// TString prt_tdcsid[prt_ntdc] ={"2000","2001","2002","2003","2004","2005","2006","2007","2008","2009",
// 			 "200a","200b","200c","200d","200e","200f","2010","2011","2012","2013",
// 			 "2014","2015","2016","2018","2019","201a","201c","2020","2023","2024",
// 			 "2025","2026","2027","2028","2029","202a","202b","202c","202d","202e","202f"
// };

// const Int_t prt_ntdc=30;  //jun2015
// TString prt_tdcsid[prt_ntdc] ={"2000","2001","2002","2003","2004","2005","2006","2007","2008","2009",
// 			 "200a","200b","200c","200d","200e","200f","2010","2011","2012","2013",
// 			 "2014","2015","2016","2018","2019","201a","201c","201d","202c","202d"
// };

// const Int_t prt_ntdc=20;  //oct2016
// TString prt_tdcsid[prt_ntdc] ={"2000","2001","2002","2003","2004","2005","2006","2007","2008","2009",
// 			       "200a","200b","200c",
// 			       "2018","201b","201c","201f","202c","202d","202d"
// };

//aug2017
TString prt_tdcsid[prt_ntdc] ={"2000","2001","2002","2003","2004","2005","2006","2007","2008","2009","200a","200b","200c","200d","200e","200f","2010","2011","2012","2013",
			       "2014","2015","2016","2017","2018","2019","201a","201b",
			       "201c","201d","201e","201f"
};


TF1 *prt_gaust;
TVector3 prt_fit(TH1F *h, Double_t range = 3, Double_t threshold=20, Double_t limit=2, Int_t peakSearch=1){
  Int_t binmax = h->GetMaximumBin();
  Double_t xmax = h->GetXaxis()->GetBinCenter(binmax);
  prt_gaust = new TF1("prt_gaust","[0]*exp(-0.5*((x-[1])/[2])^2)",xmax-range,xmax+range);
  prt_gaust->SetNpx(500);
  prt_gaust->SetParNames("const","mean","sigma");
  prt_gaust->SetLineColor(2);
  Double_t integral = h->Integral(h->GetXaxis()->FindBin(xmax-range),h->GetXaxis()->FindBin(xmax+range));
  Double_t xxmin, xxmax, sigma1(0), mean1(0), sigma2(0), mean2(0);
  xxmax = xmax;
  xxmin = xxmax;
  Int_t nfound(1);
  if(integral>threshold){
    
    if(peakSearch == 1){
      prt_gaust->SetParameter(1,xmax);
      prt_gaust->SetParameter(2,0.2);
      prt_gaust->SetParLimits(2,0.005,limit);
      h->Fit("prt_gaust","","MQN",xxmin-range, xxmax+range);
    }
    
    if(peakSearch == 2){
      nfound = prt_spect->Search(h,4,"",0.1);
      std::cout<<"nfound  "<<nfound <<std::endl;
      if(nfound==1){
	prt_gaust =new TF1("prt_gaust","gaus(0)",xmax-range,xmax+range);
	prt_gaust->SetNpx(500);
	prt_gaust->SetParameter(1,prt_spect->GetPositionX()[0]);
      }else if(nfound==2) {
	Double_t p1 = prt_spect->GetPositionX()[0];
	Double_t p2 = prt_spect->GetPositionX()[1];
	if(p1>p2) {
	  xxmax = p1;
	  xxmin = p2;
	}else {
	  xxmax = p1;
	  xxmin = p2;
	}
	prt_gaust =new TF1("prt_gaust","gaus(0)+gaus(3)",xmax-range,xmax+range);
	prt_gaust->SetNpx(500);
	prt_gaust->SetParameter(0,1000);
	prt_gaust->SetParameter(3,1000);
	
	prt_gaust->FixParameter(1,xxmin);
	prt_gaust->FixParameter(4,xxmax);
	prt_gaust->SetParameter(2,0.1);
	prt_gaust->SetParameter(5,0.1);
	h->Fit("prt_gaust","","MQN",xxmin-range, xxmax+range);
	prt_gaust->ReleaseParameter(1);
	prt_gaust->ReleaseParameter(4);
      }
    
      prt_gaust->SetParameter(2,0.2);
      prt_gaust->SetParameter(5,0.2);
    }

    h->Fit("prt_gaust","","MQN",xxmin-range, xxmax+range);
    mean1 = prt_gaust->GetParameter(1);
    sigma1 = prt_gaust->GetParameter(2);
    if(sigma1>10) sigma1=10;
    
    if(peakSearch == 2){
      mean2 = (nfound==1) ? prt_gaust->GetParameter(1) : prt_gaust->GetParameter(4);
      sigma2 = (nfound==1) ? prt_gaust->GetParameter(2) : prt_gaust->GetParameter(5);
    }
  }
  delete prt_gaust;
  return TVector3(mean1,sigma1,mean2);
}

TGraph *prt_fitslices(TH2F *hh,Double_t minrange=0, Double_t maxrange=0, Double_t fitrange=1,Int_t rebin=1){
  TH2F *h =(TH2F*) hh->Clone("h");
  h->RebinY(rebin);
  Int_t point(0);
  TGraph *gres = new TGraph();
  for (int i=0;i<h->GetNbinsY();i++){
    Double_t x = h->GetYaxis()->GetBinCenter(i);
    TH1D* hp;
    if(minrange!=maxrange){
      TCutG *cut = new TCutG("prt_onepeakcut",5);
      cut->SetVarX("y");
      cut->SetVarY("x");
      cut->SetPoint(0,minrange,-1E6);
      cut->SetPoint(1,minrange, 1E6);
      cut->SetPoint(2,maxrange, 1E6);
      cut->SetPoint(3,maxrange,-1E6);
      cut->SetPoint(4,minrange,-1E6);
    
      hp = h->ProjectionX(Form("bin%d",i+1),i+1,i+2,"[prt_onepeakcut]");
    }else{
      hp = h->ProjectionX(Form("bin%d",i+1),i+1,i+2);
    }
    
    Double_t y = prt_fit((TH1F*)hp,fitrange,100).X();
    gres->SetPoint(point++,y,x);
  }
  return gres;
}

void prt_createMap(){
  TGaxis::SetMaxDigits(4);
  Int_t seqid =-1;
  for(Int_t i=0; i<prt_maxnametdc; i++) map_tdc[i]=-1;
  for(Int_t i=0; i<prt_ntdc; i++){
    Int_t dec = TString::BaseConvert(prt_tdcsid[i],16,10).Atoi();
    map_tdc[dec]=++seqid;
  }
  
  //  for(Int_t ch=0; ch<prt_maxdircch; ch++){
  for(Int_t ch=0; ch<prt_maxch; ch++){
    Int_t mcp = ch/64;
    Int_t pix = ch%64;	
    Int_t col = pix/2 - 8*(pix/16);
    Int_t row = pix%2 + 2*(pix/16);
    pix = col+8*row;

    map_mpc[mcp][pix]=ch;
    map_mcp[ch] = mcp;
    map_pix[ch] = pix;
    map_row[ch] = row;
    map_col[ch] = col;
  } 

  for(Int_t i=0; i<5; i++){
    prt_particleArray[prt_pdg[i]]=i;
  }
  prt_particleArray[212]=2;
}

Int_t prt_getChannelNumber(Int_t tdc, Int_t tdcChannel){
  Int_t ch = -1;
  if(tdc>=0) ch = 48*tdc+tdcChannel;
  return ch;
}

Int_t prt_removeRefChannels(Int_t ch, Int_t tdcSeqId){
  return ch - tdcSeqId;
}

Int_t prt_addRefChannels(Int_t ch,Int_t tdcSeqId){
  return ch + tdcSeqId;
}

Bool_t prt_isBadChannel(Int_t ch){
  if(ch<0 || ch>prt_maxdircch) return true;
  
  // // bad pixels july15

  // if(ch==202) return true;
  // if(ch==204) return true;
  // if(ch==206) return true;
  // if(ch==830) return true;
  // if(ch==831) return true;
  // if(ch==828) return true;
  // if(ch==815) return true;
  // if(ch>383 && ch<400) return true; //dead chain

  return false;
}

// layoutId == 5    - 5 row's design for the PANDA Barrel DIRC
// layoutId == 2015 - cern 2015
// layoutId == 2016 - cern 2016
// layoutId == 2017 - cern 2016
// layoutId == 2021 - new 3.6 row's design for the PANDA Barrel DIRC
TPaletteAxis* prt_cdigi_palette;
TH1 * prt_cdigi_th;
TString prt_drawDigi(TString digidata="", Int_t layoutId = 0, Double_t maxz = 0, Double_t minz = 0){
  if(prt_geometry==2021) layoutId=2021;
  if(!prt_cdigi) prt_cdigi = new TCanvas("prt_cdigi","prt_cdigi",0,0,800,400);
  prt_cdigi->cd();
  
  if(!prt_hpglobal){
    if(layoutId==2015 ||  layoutId==5) prt_hpglobal = new TPad("P","T",0.04,0.04,0.88,0.96);
    if(layoutId==2021) prt_hpglobal = new TPad("P","T",0.12,0.02,0.78,0.98);
    if(layoutId==2016) prt_hpglobal = new TPad("P","T",0.2,0.02,0.75,0.98);
    if(layoutId==2017) prt_hpglobal = new TPad("P","T",0.15,0.02,0.80,0.98);
    if(!prt_hpglobal)  prt_hpglobal = new TPad("P","T",0.04,0.04,0.96,0.96);
    
    prt_hpglobal->SetFillStyle(0);
    prt_hpglobal->Draw();
  }
  prt_hpglobal->cd();

  
  Int_t nrow = 3, ncol = 5;

  if(layoutId ==2016) ncol=3;
  if(layoutId ==2017) ncol=4;
  if(layoutId ==2021) ncol=4;
  
  if(layoutId > 1){
    float tbw(0.02), tbh(0.01), shift(0),shiftw(0.02),shifth(0),margin(0.01);
    Int_t padi(0);
    if(!prt_hpads[0]){
      for(int i=0; i<ncol; i++){
	for(int j=0; j<nrow; j++){
	  if(j==1) shift = -0.028;
	  else shift = 0;
	  shifth=0;
	  if(layoutId == 5) {shift =0; shiftw=0.001; tbw=0.001; tbh=0.001;}
	  if(layoutId == 2021) {
	    if(i==0 && j == nrow-1) continue;
	    shift =0; shiftw=0.001; tbw=0.001; tbh=0.001;
	    if(i==0) shifth=0.167;
	  }
	  if(layoutId == 2016) {
	    shift = -0.01; shiftw=0.01; tbw=0.03; tbh=0.006;
	    if(j==1) shift += 0.015;
	  }
	  if(layoutId == 2017) {
	    margin= 0.1;
	    shift = 0; shiftw=0.01; tbw=0.005; tbh=0.006;
	    //if(j==1) shift += 0.015;
	  }
	  prt_hpads[padi] =  new TPad(Form("P%d",i*10+j),"T",
				      i/(ncol+2*margin)+tbw+shift+shiftw,
				      j/(Double_t)nrow+tbh+shifth,
				      (i+1)/(ncol+2*margin)-tbw+shift+shiftw,
				      (1+j)/(Double_t)nrow-tbh+shifth, 21);
	  prt_hpads[padi]->SetFillColor(kCyan-8);
	  prt_hpads[padi]->SetMargin(0.055,0.055,0.055,0.055);
	  prt_hpads[padi]->Draw();
	  padi++;
	}
      }
    }
  }else{
    float tbw(0.02), tbh(0.01), shift(0),shiftw(-0.02);
    Int_t padi(0);
    if(!prt_hpads[0]){
      for(int ii=0; ii<ncol; ii++){
	for(int j=0; j<nrow; j++){
	  if(j==1) shift = 0.04;
	  else shift = 0;
	  prt_hpads[padi] =  new TPad(Form("P%d",ii*10+j),"T", ii/(Double_t)ncol+tbw+shift+shiftw , j/(Double_t)nrow+tbh, (ii+1)/(Double_t)ncol-tbw+shift+shiftw, (1+j)/(Double_t)nrow-tbh, 21);
	  prt_hpads[padi]->SetFillColor(kCyan-8);
	  prt_hpads[padi]->SetMargin(0.04,0.04,0.04,0.04);
	  prt_hpads[padi]->Draw(); 
	  padi++;
	}
      }
    }

  }

  Int_t np,tmax;
  Double_t max=0;
  if(maxz==0){
    for(Int_t p=0; p<nrow*ncol;p++){
      tmax = prt_hdigi[p]->GetBinContent(prt_hdigi[p]->GetMaximumBin());
      if(max<tmax) max = tmax;
    }
  }else{
    max = maxz;
  }
  
  if(maxz==-2 && minz<-1){ // optimize range
    for(Int_t p=0; p<nrow*ncol;p++){
      tmax = prt_hdigi[p]->GetMaximum();
      if(max<tmax) max = tmax;
    }
    if(max < 100) max = 100;
    Int_t tbins = 2000;
    TH1F *h = new TH1F("","",tbins,0,max);
    for(Int_t p=0; p<nrow*ncol;p++){
      for(Int_t i=0; i<64; i++){
	Double_t val = prt_hdigi[p]->GetBinContent(i);
	if(val!=0) h->Fill(val);
      }
    }
    Double_t integral;
    for(Int_t i=0; i<tbins; i++){
      integral = h->Integral(0,i);
      if(integral>5) {
	if(minz>-3) minz = h->GetBinCenter(i);
	else minz=0;
	break;
      } 
    }

    for(Int_t i=tbins; i>0; i--){
      integral = h->Integral(i,tbins);
      if(integral>5) {
	max = h->GetBinCenter(i);
	break;
      } 
    }
  }
  Int_t nnmax(0);
  for(Int_t p=0; p<nrow*ncol;p++){
    if(layoutId == 1 || layoutId == 4)  np =p%nrow*ncol + p/3;
    else np = p;

    if(layoutId == 6 && p>10) continue;
    
    prt_hpads[p]->cd();
    //prt_hdigi[np]->Draw("col+text");
    prt_hdigi[np]->Draw("col");
    if(maxz==-1)  max = prt_hdigi[np]->GetBinContent(prt_hdigi[np]->GetMaximumBin());
    if(nnmax<prt_hdigi[np]->GetEntries()) nnmax=np;
    prt_hdigi[np]->SetMaximum(max);
    prt_hdigi[np]->SetMinimum(minz);
    for(Int_t i=1; i<=8; i++){
      for(Int_t j=1; j<=8; j++){
  	Double_t weight = (double)(prt_hdigi[np]->GetBinContent(i,j))/(double)max *255;
	if(weight>255) weight=255;
  	if(weight > 0) digidata += Form("%d,%d,%d\n", np, (j-1)*8+i-1, (Int_t)weight);
      }
    }
  }
  

  prt_cdigi->cd();
  delete prt_cdigi_palette;
  prt_cdigi_palette = new TPaletteAxis(0.82,0.1,0.86,0.90,(TH1 *) prt_hdigi[nnmax]);
  prt_cdigi_palette->Draw();
  prt_hpads[0]->cd();
    
  prt_cdigi->Modified();
  prt_cdigi->Update();
  return digidata;
}

void prt_initDigi(Int_t type=0){  
  if(type == 0){
    for(Int_t m=0; m<prt_nmcp;m++){
      if(prt_hdigi[m]) prt_hdigi[m]->Reset("M");
      else{
	prt_hdigi[m] = new TH2F( Form("mcp%d", m),Form("mcp%d", m),8,0.,8.,8,0.,8.);
	prt_hdigi[m]->SetStats(0);
	prt_hdigi[m]->SetTitle(0);
	prt_hdigi[m]->GetXaxis()->SetNdivisions(10);
	prt_hdigi[m]->GetYaxis()->SetNdivisions(10);
	prt_hdigi[m]->GetXaxis()->SetLabelOffset(100);
	prt_hdigi[m]->GetYaxis()->SetLabelOffset(100);
	prt_hdigi[m]->GetXaxis()->SetTickLength(1);
	prt_hdigi[m]->GetYaxis()->SetTickLength(1);
	prt_hdigi[m]->GetXaxis()->SetAxisColor(15);
	prt_hdigi[m]->GetYaxis()->SetAxisColor(15);
      }
    }
  }
}

void prt_resetDigi(){
  for(Int_t m=0; m<prt_nmcp;m++){	
    prt_hdigi[m]->Reset("M");
  }
}

void prt_axisHits800x500(TH2 * hist){
  hist->SetStats(0);
  hist->SetTitle(Form("%d hits",(Int_t)hist->GetEntries()));
  hist->GetXaxis()->SetTitle("z, [cm]");
  hist->GetXaxis()->SetTitleSize(0.05);
  hist->GetXaxis()->SetTitleOffset(0.8);
  hist->GetYaxis()->SetTitle("y, [cm]");
  hist->GetYaxis()->SetTitleSize(0.05);
  hist->GetYaxis()->SetTitleOffset(0.7);
}

void prt_axisAngle800x500(TH2 * hist){
  hist->SetStats(0);
  hist->SetTitle(Form("%d hits",(Int_t)hist->GetEntries()));
  hist->GetXaxis()->SetTitle("#theta, [degree]");
  hist->GetXaxis()->SetTitleSize(0.05);
  hist->GetXaxis()->SetTitleOffset(0.8);
  hist->GetYaxis()->SetTitle("photons per track, [#]");
  hist->GetYaxis()->SetTitleSize(0.05);
  hist->GetYaxis()->SetTitleOffset(0.7);
}

void prt_axisAngle800x500(TH1 * hist){
  hist->SetStats(0);
  hist->SetTitle(Form("%d hits",(Int_t)hist->GetEntries()));
  hist->GetXaxis()->SetTitle("#theta, [degree]");
  hist->GetXaxis()->SetTitleSize(0.05);
  hist->GetXaxis()->SetTitleOffset(0.8);
  hist->GetYaxis()->SetTitle("photons per track, [#]");
  hist->GetYaxis()->SetTitleSize(0.05);
  hist->GetYaxis()->SetTitleOffset(0.7);
}

void prt_axisTime800x500(TH2 * hist){
  hist->GetXaxis()->SetTitle("time, [ns]");
  hist->GetXaxis()->SetTitleSize(0.05);
  hist->GetXaxis()->SetTitleOffset(0.8);
  hist->GetYaxis()->SetTitle("entries, #");
  hist->GetYaxis()->SetTitleSize(0.05);
  hist->GetYaxis()->SetTitleOffset(0.7);
  hist->SetLineColor(1);
}

void prt_axisTime800x500(TH1 * hist, TString xtitle = "time [ns]"){
  TGaxis::SetMaxDigits(3);
  hist->GetXaxis()->SetTitle(xtitle);
  hist->GetXaxis()->SetTitleSize(0.06);
  hist->GetXaxis()->SetTitleOffset(0.8);
  hist->GetXaxis()->SetLabelSize(0.05);
  hist->GetYaxis()->SetTitle("entries [#]");
  hist->GetYaxis()->SetTitleSize(0.06);
  hist->GetYaxis()->SetTitleOffset(0.7);
  hist->GetYaxis()->SetLabelSize(0.05);
  hist->SetLineColor(1);
}

void prt_setPrettyStyle(){
  // Canvas printing details: white bg, no borders.
  gStyle->SetCanvasColor(0);
  gStyle->SetCanvasBorderMode(0);
  gStyle->SetCanvasBorderSize(0);

  // Canvas frame printing details: white bg, no borders.
  gStyle->SetFrameFillColor(0);
  gStyle->SetFrameBorderMode(0);
  gStyle->SetFrameBorderSize(0);

  // Plot title details: centered, no bg, no border, nice font.
  gStyle->SetTitleX(0.1);
  gStyle->SetTitleW(0.8);
  gStyle->SetTitleBorderSize(0);
  gStyle->SetTitleFillColor(0);

  // Font details for titles and labels.
  gStyle->SetTitleFont(42, "xyz");
  gStyle->SetTitleFont(42, "pad");
  gStyle->SetLabelFont(42, "xyz");
  gStyle->SetLabelFont(42, "pad");

  // Details for stat box.
  gStyle->SetStatColor(0);
  gStyle->SetStatFont(42);
  gStyle->SetStatBorderSize(1);
  gStyle->SetStatX(0.975);
  gStyle->SetStatY(0.9);

  // gStyle->SetOptStat(0);
}

void prt_setRootPalette(Int_t pal = 0){

  // pal =  1: rainbow\n"
  // pal =  2: reverse-rainbow\n"
  // pal =  3: amber\n"
  // pal =  4: reverse-amber\n"
  // pal =  5: blue/white\n"
  // pal =  6: white/blue\n"
  // pal =  7: red temperature\n"
  // pal =  8: reverse-red temperature\n"
  // pal =  9: green/white\n"
  // pal = 10: white/green\n"
  // pal = 11: orange/blue\n"
  // pal = 12: blue/orange\n"
  // pal = 13: white/black\n"
  // pal = 14: black/white\n"

  const Int_t NRGBs = 5;
  const Int_t NCont = 255;
  gStyle->SetNumberContours(NCont);

  if (pal < 1 && pal> 15) return;
  else pal--;

  Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
  Double_t red[15][NRGBs]   = {{ 0.00, 0.00, 0.87, 1.00, 0.51 },
			       { 0.51, 1.00, 0.87, 0.00, 0.00 },
			       { 0.17, 0.39, 0.62, 0.79, 1.00 },
			       { 1.00, 0.79, 0.62, 0.39, 0.17 },
			       { 0.00, 0.00, 0.00, 0.38, 1.00 },
			       { 1.00, 0.38, 0.00, 0.00, 0.00 },
			       { 0.00, 0.50, 0.89, 0.95, 1.00 },
			       { 1.00, 0.95, 0.89, 0.50, 0.00 },
			       { 0.00, 0.00, 0.38, 0.75, 1.00 },
			       { 0.00, 0.34, 0.61, 0.84, 1.00 },
			       { 0.75, 1.00, 0.24, 0.00, 0.00 },
			       { 0.00, 0.00, 0.24, 1.00, 0.75 },
			       { 0.00, 0.34, 0.61, 0.84, 1.00 },
			       { 1.00, 0.84, 0.61, 0.34, 0.00 },
			       { 0.00, 0.00, 0.80, 1.00, 0.80 }
  };
  Double_t green[15][NRGBs] = {{ 0.00, 0.81, 1.00, 0.20, 0.00 },		    
			       { 0.00, 0.20, 1.00, 0.81, 0.00 },
			       { 0.01, 0.02, 0.39, 0.68, 1.00 },
			       { 1.00, 0.68, 0.39, 0.02, 0.01 },
			       { 0.00, 0.00, 0.38, 0.76, 1.00 },
			       { 1.00, 0.76, 0.38, 0.00, 0.00 },
			       { 0.00, 0.00, 0.27, 0.71, 1.00 },
			       { 1.00, 0.71, 0.27, 0.00, 0.00 },
			       { 0.00, 0.35, 0.62, 0.85, 1.00 },
			       { 1.00, 0.75, 0.38, 0.00, 0.00 },
			       { 0.24, 1.00, 0.75, 0.18, 0.00 },
			       { 0.00, 0.18, 0.75, 1.00, 0.24 },
			       { 0.00, 0.34, 0.61, 0.84, 1.00 },
			       { 1.00, 0.84, 0.61, 0.34, 0.00 },
			       { 0.00, 0.85, 1.00, 0.30, 0.00 }		
  };
  Double_t blue[15][NRGBs]  = {{ 0.51, 1.00, 0.12, 0.00, 0.00 },
			       { 0.00, 0.00, 0.12, 1.00, 0.51 },
			       { 0.00, 0.09, 0.18, 0.09, 0.00 },
			       { 0.00, 0.09, 0.18, 0.09, 0.00 },
			       { 0.00, 0.47, 0.83, 1.00, 1.00 },
			       { 1.00, 1.00, 0.83, 0.47, 0.00 },
			       { 0.00, 0.00, 0.00, 0.40, 1.00 },
			       { 1.00, 0.40, 0.00, 0.00, 0.00 },
			       { 0.00, 0.00, 0.00, 0.47, 1.00 },
			       { 1.00, 0.47, 0.00, 0.00, 0.00 },
			       { 0.00, 0.62, 1.00, 0.68, 0.12 },
			       { 0.12, 0.68, 1.00, 0.62, 0.00 },
			       { 0.00, 0.34, 0.61, 0.84, 1.00 },
			       { 1.00, 0.84, 0.61, 0.34, 0.00 },
			       { 0.60, 1.00, 0.10, 0.00, 0.00 }
  };


  TColor::CreateGradientColorTable(NRGBs, stops, red[pal], green[pal], blue[pal], NCont);
 
}

#ifdef prt__sim
bool prt_init(TString inFile="../build/hits.root", Int_t bdigi=0, TString savepath=""){

  if(inFile=="") return false;
  if(savepath!="") prt_savepath=savepath;
  prt_setRootPalette(1);
  prt_createMap();
  delete prt_ch;

  prt_ch = new TChain("data");

  prt_ch->Add(inFile);
  prt_ch->SetBranchAddress("PrtEvent", &prt_event);
  
  prt_entries = prt_ch->GetEntries();
  std::cout<<"Entries in chain:  "<<prt_entries <<std::endl;
  if(bdigi == 1) prt_initDigi();
  return true;
}

void prt_nextEvent(Int_t ievent, Int_t printstep){
  prt_ch->GetEntry(ievent);
  if(ievent%printstep==0 && ievent!=0) std::cout<<"Event # "<<ievent<< " # hits "<<prt_event->GetHitSize()<<std::endl;
  if(ievent == 0){
    if(gROOT->GetApplication()){
      TIter next(gROOT->GetApplication()->InputFiles());
      TObjString *os=0;
      while((os = (TObjString*)next())){
	prt_info += os->GetString()+" ";
      }
      prt_info += "\n";
    }
    prt_info += prt_event->PrintInfo();
    prt_mom = prt_event->GetMomentum().Mag() +0.01;
    prt_theta = prt_event->GetAngle() + 0.01;
    prt_phi = prt_event->GetPhi();
    prt_geometry= prt_event->GetGeometry();
    prt_beamx= prt_event->GetBeamX();
    prt_beamz= prt_event->GetBeamZ();    
    prt_test1 = prt_event->GetTest1();
    prt_test2 = prt_event->GetTest2();
  }
  prt_particle =  prt_event->GetParticle();
  if(prt_event->GetParticle()<3000 && prt_event->GetParticle()>0){
    prt_pid=prt_particleArray[prt_event->GetParticle()];
  }
}
#endif

#ifdef prt__beam
bool prt_init(TString inFile="../build/hits.root", Int_t bdigi=0, TString savepath=""){

  if(inFile=="") return false;
  if(savepath!="") prt_savepath=savepath;
  
  prt_createMap();
  prt_setRootPalette(1);
  delete prt_ch;

  prt_ch = new TChain("data");

  prt_ch->Add(inFile);
  prt_ch->SetBranchAddress("PrtEvent", &prt_event);
  
  prt_ch->SetBranchStatus("fHitArray.fLocalPos", 0);
  prt_ch->SetBranchStatus("fHitArray.fGlobalPos", 0);
  prt_ch->SetBranchStatus("fHitArray.fDigiPos", 0);
  prt_ch->SetBranchStatus("fHitArray.fMomentum", 0);
  // prt_ch->SetBranchStatus("fHitArray.fPosition", 0);
  
  prt_ch->SetBranchStatus("fHitArray.fParentParticleId", 0);
  prt_ch->SetBranchStatus("fHitArray.fNreflectionsInPrizm", 0);
  prt_ch->SetBranchStatus("fHitArray.fPathInPrizm", 0);
  prt_ch->SetBranchStatus("fHitArray.fCherenkovMC", 0);

  prt_ch->SetBranchStatus("fPosition", 0);

  prt_entries = prt_ch->GetEntries();
  std::cout<<"Entries in chain: "<<prt_entries <<std::endl;
  if(bdigi == 1) prt_initDigi();
  return true;
}

void prt_nextEvent(Int_t ievent, Int_t printstep){
  prt_ch->GetEntry(ievent);
  if(ievent%printstep==0 && ievent!=0) cout<<"Event # "<<ievent<< " # hits "<<prt_event->GetHitSize()<<endl;
  if(ievent == 0){
    if(gROOT->GetApplication()){
      prt_info += "beam test";
      TIter next(gROOT->GetApplication()->InputFiles());
      TObjString *os=0;
      while((os = (TObjString*)next())){
	prt_info += os->GetString()+" ";
      }
      prt_info += "\n";
    }
    prt_info += prt_event->PrintInfo();
    prt_mom = prt_event->GetMomentum().Mag() +0.01;
    prt_theta = prt_event->GetAngle() + 0.01;
    prt_phi = prt_event->GetPhi();
    prt_geometry= prt_event->GetGeometry();
    prt_beamx= prt_event->GetBeamX();
    prt_beamz= prt_event->GetBeamZ();
    prt_test1 = prt_event->GetTest1();
    prt_test2 = prt_event->GetTest2();
  }
  prt_particle =  prt_event->GetParticle();
  if(prt_event->GetParticle()<3000 && prt_event->GetParticle()>0){
    prt_pid=prt_particleArray[prt_event->GetParticle()];
  }
}
#endif

TString prt_randstr(Int_t len = 10){
  gSystem->Sleep(1500);
  srand (time(NULL));
  TString str = ""; 
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    str += alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return str;
}

Int_t prt_getColorId(Int_t ind, Int_t style =0){
  Int_t cid = 1;
  if(style==0) {
    cid=ind+1;
    if(cid==5) cid =8;
    if(cid==3) cid =kOrange+2;
  }
  if(style==1) cid=ind+300;
  return cid;
}

Int_t prt_shiftHist(TH1F *hist, Double_t double_shift){
  Int_t bins=hist->GetXaxis()->GetNbins();
  Double_t xmin=hist->GetXaxis()->GetBinLowEdge(1);
  Double_t xmax=hist->GetXaxis()->GetBinUpEdge(bins);
  double_shift=double_shift*(bins/(xmax-xmin));
  Int_t shift=0;
  if(double_shift<0) shift=TMath::FloorNint(double_shift);
  if(double_shift>0) shift=TMath::CeilNint(double_shift);
  if(shift==0) return 0;
  if(shift>0){
    for(Int_t i=1; i<=bins; i++){
      if(i+shift<=bins) hist->SetBinContent(i,hist->GetBinContent(i+shift));
      if(i+shift>bins) hist->SetBinContent(i,0);
    }
    return 0;
  }
  if(shift<0){
    for(Int_t i=bins; i>0; i--){
      if(i+shift>0) hist->SetBinContent(i,hist->GetBinContent(i+shift));
      if(i+shift<=0) hist->SetBinContent(i,0);
    }    
    return 0;
  }
  return 1;
} 

void prt_addInfo(TString str){
  prt_info += str+"\n";
}

void prt_writeInfo(TString filename){
  std::ofstream myfile;
  myfile.open (filename);
  myfile << prt_info+"\n";
  myfile.close();
}

void prt_writeString(TString filename, TString str){
  std::ofstream myfile;
  myfile.open (filename);
  myfile << str+"\n";
  myfile.close();
}

TString prt_createDir(TString inpath=""){
  if(inpath != "") prt_savepath = inpath;
  TString finalpath = prt_savepath;

  if(finalpath =="") return "";
  
  if(prt_savepath.EndsWith("auto")) {
    TString dir = prt_savepath.ReplaceAll("auto","data");
    gSystem->mkdir(dir);
    TDatime *time = new TDatime();
    TString path(""), stime = Form("%d.%d.%d", time->GetDay(),time->GetMonth(),time->GetYear()); 
    gSystem->mkdir(dir+"/"+stime);
    for(Int_t i=0; i<1000; i++){
      path = stime+"/"+Form("arid-%d",i);
      if(gSystem->mkdir(dir+"/"+path)==0) break;
    }
    gSystem->Unlink(dir+"/last");
    gSystem->Symlink(path, dir+"/last");
    finalpath = dir+"/"+path;
    prt_savepath=finalpath;
  }else{
    gSystem->mkdir(prt_savepath,kTRUE);
  }
  
  prt_writeInfo(finalpath+"/readme");
  return finalpath;
}

void prt_save(TPad *c= NULL,TString path="", TString name="", Int_t what=0, Int_t style=0){
  if(c && path != "") {
    gROOT->SetBatch(1);
    Int_t w = 800, h = 400;
    if(style != -1){
      if(style == 1) {w = 800; h = 500;}
      if(style == 2) {w = 800; h = 600;}
      if(style == 3) {w = 800; h = 400;}
      if(style == 5) {w = 800; h = 900;} 
      if(style == 0){ 
	w = ((TCanvas*)c)->GetWindowWidth();
	h = ((TCanvas*)c)->GetWindowHeight();
      }

      // TCanvas *cc = new TCanvas(TString(c->GetName())+"exp","cExport",0,0,w,h);
      // cc = (TCanvas*) c->DrawClone();
      // cc->SetCanvasSize(w,h);

      TCanvas *cc;
      cc = (TCanvas*) c;
      if(TString(c->GetName()).Contains("hp") || TString(c->GetName()).Contains("cdigi")) cc = (TCanvas*) c;
      else {
      	cc = new TCanvas(TString(c->GetName())+"exp","cExport",0,0,w,h);
      	cc = (TCanvas*) c->DrawClone();      
      	cc->SetCanvasSize(w,h);
      }
      
      if(style == 0) {
	if(fabs(cc->GetBottomMargin()-0.1)<0.001) cc->SetBottomMargin(0.12);
	TIter next(cc->GetListOfPrimitives());
	TObject *obj;
	
	while((obj = next())){
	  if(obj->InheritsFrom("TH1")){
	    TH1F *hh = (TH1F*)obj;
	    hh->GetXaxis()->SetTitleSize(0.06);
	    hh->GetYaxis()->SetTitleSize(0.06);

	    hh->GetXaxis()->SetLabelSize(0.05);
	    hh->GetYaxis()->SetLabelSize(0.05);
	    
	    hh->GetXaxis()->SetTitleOffset(0.85);
	    hh->GetYaxis()->SetTitleOffset(0.76);

	    if(fabs(cc->GetBottomMargin()-0.12)<0.001){
	      TPaletteAxis *palette = (TPaletteAxis*)hh->GetListOfFunctions()->FindObject("palette");
	      if(palette) {
		palette->SetY1NDC(0.12);
		cc->Modified();
	      }
	    }
	  }
	  if(obj->InheritsFrom("TGraph")){
	    TGraph *gg = (TGraph*)obj;
	    gg->GetXaxis()->SetLabelSize(0.05);
	    gg->GetXaxis()->SetTitleSize(0.06);
	    gg->GetXaxis()->SetTitleOffset(0.84);

	    gg->GetYaxis()->SetLabelSize(0.05);
	    gg->GetYaxis()->SetTitleSize(0.06);
	    gg->GetYaxis()->SetTitleOffset(0.7);
	  }
	  if(obj->InheritsFrom("TF1")){
	    TF1 *f = (TF1*)obj;
	    f->SetNpx(500);
	  }
	}
      }
      
      cc->Modified();
      cc->Update();
      cc->SaveAs(path+"/"+name+".png");
      if(what==0) cc->Print(path+"/"+name+".eps");
      if(what==0) cc->Print(path+"/"+name+".pdf");
      if(what==0 || what==2) cc->Print(path+"/"+name+".root");
    }else{
      c->SetCanvasSize(w,h);
      c->Print(path+"/"+name+".png");
      if(what==0) c->Print(path+"/"+name+".pdf");
      if(what==0 || what==2) c->Print(path+"/"+name+".root");
    }		    
    gROOT->SetBatch(0);
  }
}

TString prt_createSubDir(TString dir="dir"){
  gSystem->mkdir(dir);
  return dir;
}

TList *prt_canvaslist;
void prt_canvasAdd(TString name="c",Int_t w=800, Int_t h=600){
  if(!prt_canvaslist) prt_canvaslist = new TList();
  TCanvas *c = new TCanvas(name,name,0,0,w,h); 
  prt_canvaslist->Add(c);
}

void prt_canvasAdd(TCanvas *c){
  if(!prt_canvaslist) prt_canvaslist = new TList();
  c->cd();
  prt_canvaslist->Add(c);
}

void prt_canvasCd(TString name="c"){
  
}

TCanvas *prt_canvasGet(TString name="c"){
  TIter next(prt_canvaslist);
  TCanvas *c=0;
  while((c = (TCanvas*) next())){
    if(c->GetName()==name || name=="*") break;
  }
  return c;
}

void prt_canvasDel(TString name="c"){
  TIter next(prt_canvaslist);
  TCanvas *c=0;
  while((c = (TCanvas*) next())){
    if(c->GetName()==name || name=="*") prt_canvaslist->Remove(c);
    c->Delete();
  }
}

// style = 0 - for web blog
// style = 1 - for talk 
// what = 0 - save in png, pdf, eps, root formats
// what = 1 - save in png format
// what = 2 - save in png and root format
void prt_canvasSave(Int_t what=1, Int_t style=0){
  TIter next(prt_canvaslist);
  TCanvas *c=0;
  TString path = prt_createDir();
  while((c = (TCanvas*) next())){
    prt_save(c, path, c->GetName(), what,style);
    prt_canvaslist->Remove(c);
  }
}

void prt_waitPrimitive(TString name, TString prim=""){
  TIter next(prt_canvaslist);
  TCanvas *c=0;
  while((c = (TCanvas*) next())){
    if(TString(c->GetName())==name){
      c->Modified(); 
      c->Update(); 
      c->WaitPrimitive(prim);
    }
  }
}

Double_t prt_integral(TH1F *h,Double_t xmin, Double_t xmax){
  TAxis *axis = h->GetXaxis();
  Int_t bmin = axis->FindBin(xmin); //in your case xmin=-1.5
  Int_t bmax = axis->FindBin(xmax); //in your case xmax=0.8
  Double_t integral = h->Integral(bmin,bmax);
  integral -= h->GetBinContent(bmin)*(xmin-axis->GetBinLowEdge(bmin))/axis->GetBinWidth(bmin);
  integral -= h->GetBinContent(bmax)*(axis->GetBinUpEdge(bmax)-xmax)/axis->GetBinWidth(bmax);
  return integral;
}

void prt_normalize(TH1F* hists[],Int_t size){
  // for(Int_t i=0; i<size; i++){
  //   hists[i]->Scale(1/hists[i]->Integral(), "width"); 
  // }
  
  Double_t max = 0;
  Double_t min = 0;
  for(Int_t i=0; i<size; i++){
    Double_t tmax =  hists[i]->GetBinContent(hists[i]->GetMaximumBin());
    Double_t tmin = hists[i]->GetMinimum();
    if(tmax>max) max = tmax;
    if(tmin<min) min = tmin;
  }
  max += 0.05*max;
  for(Int_t i=0; i<size; i++){
    hists[i]->GetYaxis()->SetRangeUser(min,max);
  }
}

void prt_normalize(TH1F* h1,TH1F* h2){
  Double_t max = (h1->GetMaximum()>h2->GetMaximum())? h1->GetMaximum() : h2->GetMaximum();
  max += max*0.1;
  h1->GetYaxis()->SetRangeUser(0,max);
  h2->GetYaxis()->SetRangeUser(0,max);
}


// just x for now
TGraph* prt_smooth(TGraph* g,Int_t smoothness=1){
  Double_t x, y;
  Int_t n = g->GetN();
  TH1F *h = new TH1F("h","h",g->GetN(),0,n);
  TGraph *gr = new TGraph();
  gr->SetName(g->GetName());
  for(auto i=0; i<n; i++){
    g->GetPoint(i,x,y);
    h->Fill(i,x);
  }

  h->Smooth(smoothness);
  
  for(auto i=0;i<n;i++){
    g->GetPoint(i,x,y);
    gr->SetPoint(i,h->GetBinContent(i),y);
  }
  return gr;
}



