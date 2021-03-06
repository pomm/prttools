#define prt__beam
#include "../prtdirc/src/PrtHit.h"
#include "../prtdirc/src/PrtEvent.h"
#include "prttools.C"

TSpectrum *spect = new TSpectrum(2);
TF1 * fitpdf(TH1F *h){
  TF1 *gaust = NULL;
  Double_t rmin(0),rmax(60);
  Double_t integral = h->Integral(h->GetXaxis()->FindBin(rmin),h->GetXaxis()->FindBin(rmax));
  if(h->GetEntries()>50){
    Int_t nfound = spect->Search(h,2,"",0.1);
    std::cout<<"nfound  "<<nfound <<std::endl;
    if(nfound==1){
      gaust =new TF1("gaust","gaus(0)",rmin,rmax);
      gaust->SetNpx(500);
      gaust->FixParameter(1,spect->GetPositionX()[0]);
      gaust->SetParameter(2,0.3);
      gaust->SetParLimits(2,0.2,1);
    }
    if(nfound==2){
      gaust =new TF1("gaust","gaus(0)+gaus(3)",rmin,rmax);
      gaust->SetNpx(500);
      gaust->FixParameter(1,spect->GetPositionX()[0]);
      gaust->FixParameter(4,spect->GetPositionX()[1]);
      gaust->SetParameter(2,0.3);
      gaust->SetParameter(5,0.3);
      gaust->SetParLimits(2,0.2,1);
      gaust->SetParLimits(5,0.2,1);
    
      std::cout<<spect->GetPositionX()[0]<< " "<<spect->GetPositionX()[1] <<std::endl;
    
    }
    h->Fit("gaust","","MQN",0,60);
  }else{
    gaust =new TF1("gaust","pol0",rmin,rmax);
    gaust->FixParameter(0,0);
  }
	 
  return gaust;
}

void createPdf(TString path="", Int_t normtype=1 ,Bool_t save=false, Int_t aentries=-1){

  if(!prt_init(path,1,"data/cratePdf_s311")) return;
  gStyle->SetOptStat(0);

  Int_t totalmcp[5][prt_nmcp], totalmcpr[5][prt_nmcp];
  for(Int_t i=0; i<5; i++){
    for(Int_t m=0; m<prt_nmcp; m++){
      totalmcp[i][m]=0;
      totalmcpr[i][m]=0;
    }
  }
  
  TH1F *hlef[prt_maxch], *hles[prt_maxch];

  for(Int_t i=0; i<prt_maxch; i++){
    hlef[i] = new TH1F(Form("lef_%d",i),"pdf;LE time [ns]; entries [#]",1000,0,50);
    hles[i] = new TH1F(Form("les_%d",i),"pdf;LE time [ns]; entries [#]",1000,0,50);
  }
  TH1F *hnphf = new TH1F("hnphf","hnphf",200,0,200);
  TH1F *hnphs = new TH1F("hnphs","hnphs",200,0,200);

  Int_t trigT1(816);
  Int_t trigT2(817);
  Int_t trigT3h(818);
  Int_t trigT3v(819);
  Int_t trigTof1(1392);
  Int_t trigTof2(1398);
  
  Double_t time;
  PrtHit hit;
  Int_t totalf(0),totals(0),ch,entries = prt_entries;
  if(aentries>=0) entries = aentries;
  Int_t start = 0;
  if(path.Contains("S.root")) start=4000;
  if(path.Contains("C.root")) start=100000;
  
  for (Int_t ievent=start; ievent<entries; ievent++){ //entries
    prt_nextEvent(ievent,1000);

    Int_t nHits =prt_event->GetHitSize();
    Double_t tof = prt_event->GetTest1();
    Int_t pid=prt_event->GetParticle();
    
    if(prt_event->GetType()==0){
      // if(fabs(prt_event->GetMomentum().Mag()-7)<0.1){
      // 	if( prt_event->GetParticle()==2212 && tof<32.4 ) continue;
      // 	if( prt_event->GetParticle()==211  && tof>31.9 ) continue;
      // }
      
      Bool_t t1(1),tof1(1), tof2(1);
      Bool_t t2(0),t3h(0),t3v(0); 
      Bool_t hodo1(0), hodo2(0);
      
      for(Int_t h=0; h<nHits; h++) {
      	hit = prt_event->GetHit(h);
      	Int_t gch=hit.GetChannel();

	if(gch==trigT2)
	  t2=true;
	if(gch==trigT3h)
	  t3h=true;
	if(gch==trigT3v)
	  t3v=true;


	//	if(gch>=1348 && gch<=1352)
	if(gch>=1347 && gch<=1353)
	  hodo1=true;
	//if(gch>=1369 && gch<=1370)
	  hodo2=true;
      }

      if(!(t1 && t2 && t3h && t3v && tof1 && tof2 && hodo1 && hodo2)) continue;
    }
    
    Int_t goodhits(0);
    Int_t mult[prt_maxch];
    memset(mult, 0, sizeof(mult));
    
    for(Int_t i=0; i<nHits; i++){
      hit = prt_event->GetHit(i);
      Int_t mcp = hit.GetMcpId();
      Int_t pix = hit.GetPixelId()-1;      
      ch=map_mpc[mcp][pix];
      if(ch>prt_maxdircch) continue;
      time=hit.GetLeadTime();
      //if(prt_event->GetType()!=0) time += gRandom->Gaus(0,0.3);
      //Double_t tot= hit.GetTotTime();
      //if(tot<2 || tot>8) continue;
      
      if(++mult[ch]>1) continue;
      if(time<0 || time>50) continue;
      goodhits++;

      if(pid==2212){
	totalmcp[4][mcp]++;
	totalmcpr[4][mcp%3]++;
	if(normtype) totalf++;
	hlef[ch]->Fill(time);
      }
      if(pid==211){
	totalmcp[2][mcp]++;
	totalmcpr[2][mcp%3]++;
	if(normtype) totals++;
	hles[ch]->Fill(time);
      }
      prt_hdigi[mcp]->Fill(pix%8, pix/8);
    }
    
    if(pid==2212){
      hnphf->Fill(goodhits);
      if(normtype==0) totalf++;
    }
    if(pid==211){
      hnphs->Fill(goodhits);
      if(normtype==0) totals++;
    }
  }

  std::cout<<"#1 "<< totalf <<"  #2 "<<totals <<std::endl;

  TCanvas *cExport = new TCanvas("cExport","cExport",0,0,800,400);
  
  if(totalf>0 && totals>0) {
    path.ReplaceAll("*","");
    path.ReplaceAll(".root",Form(".pdf%d.root",normtype));
    if(aentries>=0) path.ReplaceAll(".root",Form("_%d.root",aentries)); 
    
    TFile efile(path,"RECREATE");
    hnphf->Scale(1/(Double_t)(hnphf->GetMaximum()));
    hnphs->Scale(1/(Double_t)(hnphs->GetMaximum()));
    hnphf->Write();
    hnphs->Write();
    for(Int_t i=0; i<prt_maxch; i++){
      Int_t mcp = i/64;

      if(normtype<=1){
	hlef[i]->Scale(1/(Double_t)totalf);
	hles[i]->Scale(1/(Double_t)totals);
      }
      
      if(normtype==2){
	hlef[i]->Scale(1/(Double_t)totalmcp[4][mcp]);
	hles[i]->Scale(1/(Double_t)totalmcp[2][mcp]);
      }
	    
      if(normtype==3){
	hlef[i]->Scale(1/(Double_t)totalmcpr[4][mcp%3]);
	hles[i]->Scale(1/(Double_t)totalmcpr[2][mcp%3]);
      }

      // hlef[i]->Scale(1/(Double_t)(hlef[i]->GetEntries()));
      // hles[i]->Scale(1/(Double_t)(hlef[i]->GetEntries()));

      // Double_t mm = hles[i]->Integral();
      // if(hlef[i]->Integral()>hles[i]->Integral()) mm=hlef[i]->Integral();
      
      // hlef[i]-> Scale(1./mm, "width"); 
      // hles[i]-> Scale(1./mm, "width");
      
      // TF1 *f = fitpdf(hlef[i]);
      // TF1 *s = fitpdf(hles[i]);
      // for(Int_t p=0; p<f->GetNpar(); p++) f->FixParameter(p,f->GetParameter(p));
      // for(Int_t p=0; p<s->GetNpar(); p++) s->FixParameter(p,s->GetParameter(p));
      // f->SetName(Form("ff_%d",i));
      // s->SetName(Form("fs_%d",i));
      // f->Write();
      // s->Write();
      
      hlef[i]->SetName(Form("hf_%d",i));
      hles[i]->SetName(Form("hs_%d",i));
      hlef[i]->Write();
      hles[i]->Write();
      
      if(save){
	cExport->cd();
      	hlef[i]->GetXaxis()->SetRangeUser(0,50);
	hles[i]->GetXaxis()->SetRangeUser(0,50);
	prt_normalize(hlef[i],hles[i]);
	hlef[i]->SetLineColor(2);
      	hles[i]->SetLineColor(4);
	hlef[i]->Draw();
      	hles[i]->Draw("same");
	cExport->SetName(Form("pdf_mcp%d_pix_%d",map_mcp[i],map_pix[i]));
	prt_canvasAdd(cExport);
      	prt_canvasSave(1,0);
      }
    }
    
    efile.Write();
    efile.Close();
  }

 
  prt_canvasAdd("le",800,500);
  hlef[308]->SetLineColor(2);
  hlef[308]->Draw();
  hles[308]->SetLineColor(4);
  hles[308]->Draw("same");
  
  //  writeString(fSavePath+"/digi.csv", drawDigi("m,p,v\n",2,-2,-2));
  //prt_writeString(prt_savepath+"/digi.csv", prt_drawDigi("m,p,v\n",prt_geometry,0,0));
  //prt_cdigi->SetName("hits");
  //prt_canvasAdd(prt_cdigi);
  
  prt_canvasSave(1,0);
}
