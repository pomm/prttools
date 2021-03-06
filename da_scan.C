#include "prttools.C"
void da_scan(TString inFile = "r_spr.root"){
  
  TString outdir=inFile;outdir.Remove(outdir.Last('/'));
  TString sfile=inFile; sfile.Remove(0,sfile.Last('/')+1);
  TString sstudy=outdir; sstudy.Remove(0,sstudy.Last('/'));
  prt_savepath = outdir+sstudy+"r";
  if(inFile.Contains("S.root"))  prt_savepath = outdir+sstudy+"s";
  TString outFile=outdir+"/c"+sfile;

  TChain ch("dirc"); ch.Add(inFile);
  Double_t cangle,spr,trr,nph,par1,par2,par3,par4,par5,par6,test1,test2,theta,phi; 
  
  TGraph *gSpr = new TGraph();
  TGraph *gNph = new TGraph();
  TGraph *gTrr = new TGraph();

  ch.SetBranchAddress("spr",&spr);
  ch.SetBranchAddress("trr",&trr);
  ch.SetBranchAddress("nph",&nph);
  ch.SetBranchAddress("cangle",&cangle);
  ch.SetBranchAddress("par4",&par4);
  ch.SetBranchAddress("par5",&par5);
  ch.SetBranchAddress("par6",&par6);
  ch.SetBranchAddress("test1",&test1);
  ch.SetBranchAddress("test2",&test2);
  ch.SetBranchAddress("theta",&theta);
  ch.SetBranchAddress("phi",&phi);
  
  Int_t nent = ch.GetEntries();
  std::cout<<"# entries  "<< nent <<std::endl;
  std::cout<<"infor  "<< ch.GetTree()->GetTitle()<<std::endl;

  Int_t it(0);
  for (Int_t i = 0; i < nent; i++) {
    ch.GetEvent(i);
    if(spr==0 || theta>156) continue;
    gSpr->SetPoint(it,theta,TMath::Abs(spr));
    gNph->SetPoint(it,theta,nph);
    gTrr->SetPoint(it,theta,TMath::Abs(trr));
    it++;
  }
  gSpr->Sort();
  gNph->Sort();
  gTrr->Sort();

  gSpr->SetLineColor(38);
  gNph->SetLineColor(38);
  gTrr->SetLineColor(38);
  gSpr->SetMarkerStyle(20);
  gNph->SetMarkerStyle(20);
  gTrr->SetMarkerStyle(20);
  gSpr->SetMarkerSize(0.7);
  gNph->SetMarkerSize(0.7);
  gTrr->SetMarkerSize(0.7);
  gNph->GetYaxis()->SetRangeUser(0,120);
  gSpr->GetYaxis()->SetRangeUser(0,30);
  gTrr->GetYaxis()->SetRangeUser(0,5);

  gNph->GetXaxis()->SetLimits(10,165);
  gSpr->GetXaxis()->SetLimits(10,165);
  gTrr->GetXaxis()->SetLimits(10,165);
  
  gNph->GetXaxis()->SetRangeUser(10,165);
  gSpr->GetXaxis()->SetRangeUser(10,165);
  gTrr->GetXaxis()->SetRangeUser(10,165);

  gSpr->GetYaxis()->SetTitle("SPR [mrad]");
  gNph->GetYaxis()->SetTitle("multiplicity [#]");
  gTrr->GetYaxis()->SetTitle("#sigma_{#theta_{C} tr} [mrad]");
  
  gSpr->GetXaxis()->SetLabelSize(0.05);
  gSpr->GetXaxis()->SetTitleSize(0.06);
  gSpr->GetXaxis()->SetTitleOffset(0.84);

  gTrr->GetXaxis()->SetLabelSize(0.05);
  gTrr->GetXaxis()->SetTitleSize(0.06);
  gTrr->GetXaxis()->SetTitleOffset(0.84);

  gNph->GetXaxis()->SetLabelSize(0.05);
  gNph->GetXaxis()->SetTitleSize(0.06);
  gNph->GetXaxis()->SetTitleOffset(0.84);

  gSpr->GetYaxis()->SetLabelSize(0.05);
  gSpr->GetYaxis()->SetTitleSize(0.06);
  gSpr->GetYaxis()->SetTitleOffset(0.7);

  gTrr->GetYaxis()->SetLabelSize(0.05);
  gTrr->GetYaxis()->SetTitleSize(0.06);
  gTrr->GetYaxis()->SetTitleOffset(0.7);

  gNph->GetYaxis()->SetLabelSize(0.05);
  gNph->GetYaxis()->SetTitleSize(0.06);
  gNph->GetYaxis()->SetTitleOffset(0.7);


  gSpr->GetXaxis()->SetTitle("#theta_{track} [degree]");
  gNph->GetXaxis()->SetTitle("#theta_{track} [degree]");
  gTrr->GetXaxis()->SetTitle("#theta_{track} [degree]");

  TFile *file = new TFile(outFile,"RECREATE");
  TCanvas* c1 = new TCanvas("spr","spr",800,400);c1->SetBottomMargin(0.12);
  gSpr->Draw("APL");
  prt_canvasAdd(c1);
  TCanvas* c2 = new TCanvas("nph","nph",800,400);c2->SetBottomMargin(0.12);
  gNph->Draw("APL");
  prt_canvasAdd(c2);
  TCanvas* c3 = new TCanvas("trr","trr",800,400);c3->SetBottomMargin(0.12);
  gTrr->Draw("APL");
  prt_canvasAdd(c3);

  prt_canvasSave(1,0);
   
  file->cd();
  c1->Write();
  c2->Write();
  c3->Write();
  file->Close();
  
}
