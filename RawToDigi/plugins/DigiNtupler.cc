// -*- C++ -*-
//
// Package:    HGCal/DigiNtupler
// Class:      DigiNtupler
//
/**\class DigiNtupler DigiNtupler.cc HGCal/DigiNtupler/plugins/DigiNtupler.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Si Xie, Artur Apresyan, Rajdeep Mohan Chatterjee
//         Created:  Mon, 31 Mar 2016 
//
//


// system include files
#include <memory>
#include <iostream>
#include "TH2Poly.h"
#include "TH1F.h"
#include "TProfile.h"
#include "TTree.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "HGCal/DataFormats/interface/HGCalTBRecHitCollections.h"
#include "HGCal/DataFormats/interface/HGCalTBDetId.h"
#include "HGCal/DataFormats/interface/HGCalTBRecHit.h"
#include "HGCal/Geometry/interface/HGCalTBCellVertices.h"
#include "HGCal/Geometry/interface/HGCalTBTopology.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "HGCal/DataFormats/interface/HGCalTBDataFrameContainers.h"
#include "HGCal/CondObjects/interface/HGCalElectronicsMap.h"
#include "HGCal/DataFormats/interface/HGCalTBElectronicsId.h"
//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<> and also remove the line from
// constructor "usesResource("TFileService");"
// This will improve performance in multithreaded jobs.

class DigiNtupler : public edm::one::EDAnalyzer<edm::one::SharedResources>
{

public:
	explicit DigiNtupler(const edm::ParameterSet&);
	~DigiNtupler();
	static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
private:
	virtual void beginJob() override;
	void analyze(const edm::Event& , const edm::EventSetup&) override;
	virtual void endJob() override;
	// ----------member data ---------------------------
	bool DEBUG = false;
	HGCalTBTopology IsCellValid;
	HGCalTBCellVertices TheCell;
	int sensorsize = 128;// The geometry for a 256 cell sensor hasnt been implemted yet. Need a picture to do this.
	std::vector<std::pair<double, double>> CellXY;
	std::pair<double, double> CellCentreXY;
	std::vector<std::pair<double, double>>::const_iterator it;
	const static int NSAMPLES = 2;
	const static int NLAYERS  = 1;
	TH2Poly *h_digi_layer[NSAMPLES][NLAYERS][1024];
 //       TH2Poly *h_digi_layer_Raw[NSAMPLES][NLAYERS][512]; 
        TH1F* Sum_Hist_cells_SKI1[1024]; 
	const static int cellx = 15;
	const static int celly = 15;
	int Sensor_Iu = 0;
	int Sensor_Iv = 0;
	char name[50], title[50];

        TTree *outputTree = 0;
        uint eventNum;
        float XData[128];
        float YData[128];
        float UData[128];
        float VData[128];
        float ADCData[128];
        float ADCHighData[128];
        int channelNum[128];
        uint maxChannelIndex;
        float maxADC;
        float maxX;
        float maxY;
        float ped1;
        float ped2;
        float ADCSum1;
        float ADCSum2;
        float centroidX;
        float centroidY;
	int cellTypeCode[128]; // 0->unknown; 1->half; 2->calibration; 3->full size non calibration
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
DigiNtupler::DigiNtupler(const edm::ParameterSet& iConfig)
{
  //now do what ever initialization is needed
  usesResource("TFileService");
  edm::Service<TFileService> fs;
  consumesMany<SKIROC2DigiCollection>();
  const int HalfHexVertices = 4;
  double HalfHexX[HalfHexVertices] = {0.};
  double HalfHexY[HalfHexVertices] = {0.};
  const int FullHexVertices = 6;
  double FullHexX[FullHexVertices] = {0.};
  double FullHexY[FullHexVertices] = {0.};
  int iii = 0;
  for(int nsample = 0; nsample < NSAMPLES; nsample++) {
    for(int nlayers = 0; nlayers < NLAYERS; nlayers++) {
      for(int eee = 0; eee< 1024; eee++){
	//Booking a "hexagonal" histograms to display the sum of Digis for NSAMPLES, in 1 SKIROC in 1 layer. To include all layers soon. Also the 1D Digis per cell in a sensor is booked here for NSAMPLES.
	sprintf(name, "FullLayer_ADC%i_Layer%i_Event%i", nsample, nlayers + 1,eee);
	sprintf(title, "Sum of adc counts per cell for ADC%i Layer%i Event%i", nsample, nlayers + 1,eee);
	h_digi_layer[nsample][nlayers][eee] = new TH2Poly();
	h_digi_layer[nsample][nlayers][eee]->SetName(name);
	h_digi_layer[nsample][nlayers][eee]->SetTitle(title);

	sprintf(name, "SumCellHist_SKI1_Event%i",eee);
	sprintf(title, "Sum of adc counts of cells  Event%i",eee);
	Sum_Hist_cells_SKI1[eee] = new TH1F(name,title,4096, 0 , 4095);
	for(int iv = -7; iv < 8; iv++) {
	  for(int iu = -7; iu < 8; iu++) {

	    CellXY = TheCell.GetCellCoordinatesForPlots(nlayers, Sensor_Iu, Sensor_Iv, iu, iv, sensorsize);
	    int NumberOfCellVertices = CellXY.size();
	    iii = 0;
	    if(NumberOfCellVertices == 4) {
	      for(it = CellXY.begin(); it != CellXY.end(); it++) {
		HalfHexX[iii] =  it->first;
		HalfHexY[iii++] =  it->second;
	      }
	      //Somehow cloning of the TH2Poly was not working. Need to look at it. Currently physically booked another one.
	      h_digi_layer[nsample][nlayers][eee]->AddBin(NumberOfCellVertices, HalfHexX, HalfHexY);

	    } else if(NumberOfCellVertices == 6) {
	      iii = 0;
	      for(it = CellXY.begin(); it != CellXY.end(); it++) {
		FullHexX[iii] =  it->first;
		FullHexY[iii++] =  it->second;
	      }
	      h_digi_layer[nsample][nlayers][eee]->AddBin(NumberOfCellVertices, FullHexX, FullHexY);
	      //                                                h_digi_layer_Raw[nsample][nlayers][eee]->AddBin(NumberOfCellVertices, FullHexX, FullHexY);

	    }

	  }//loop over iu
	}//loop over iv
      }//loop over number of events
    }//loop over nlayers
  }//loop over nsamples



  //create ntuple here
  outputTree = fs->make<TTree>("HGCTBTree", "HGCTBTree");

}//contructor ends here


DigiNtupler::~DigiNtupler()
{

  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called for each event  ------------
void
DigiNtupler::analyze(const edm::Event& event, const edm::EventSetup& setup)
{
  eventNum = event.id().event();
  using namespace edm;
  using namespace std;
  std::vector<edm::Handle<SKIROC2DigiCollection> > ski;
  event.getManyByType(ski);
  //        int Event = event.id().event();
  
  //initialize maps
  std::map<std::pair<int,int>, float> ADCMAP;
  std::map<std::pair<int,int>, std::pair<float,float> > XYMAP;

  //initialize output
  ADCSum1 = 0;
  ADCSum2 = 0;
  centroidX = -999;
  centroidY = -999;
  maxADC = -1;
  maxX = -999;
  maxY = -999;
  ped1 = -999;
  ped2 = -999;
  maxChannelIndex = 0;

  if(!ski.empty()) {
    std::vector<edm::Handle<SKIROC2DigiCollection> >::iterator i;
    double Average_Pedestal_Per_Event1 = 0, Average_Pedestal_Per_Event2 = 0;
    int Cell_counter1 = 0, Cell_counter2 = 0;
    int channelIndex  = 0;
    //                int counter1=0, counter2=0;
    for(i = ski.begin(); i != ski.end(); i++) {
      const SKIROC2DigiCollection& Coll = *(*i);
      if(DEBUG) cout << "SKIROC2 Digis: " << i->provenance()->branchName() << endl;
      //////////////////////////////////Evaluate average pedestal per event to subtract out//////////////////////////////////
      for(SKIROC2DigiCollection::const_iterator k = Coll.begin(); k != Coll.end(); k++) {
	const SKIROC2DataFrame& SKI_1 = *k ;
	int n_layer = (SKI_1.detid()).layer();
	int n_sensor_IU = (SKI_1.detid()).sensorIU();
	int n_sensor_IV = (SKI_1.detid()).sensorIV();
	int n_cell_iu = (SKI_1.detid()).iu();
	int n_cell_iv = (SKI_1.detid()).iv();
	if(DEBUG) cout << endl << " Layer = " << n_layer << " Sensor IU = " << n_sensor_IU << " Sensor IV = " << n_sensor_IV << " Cell iu = " << n_cell_iu << " Cell iu = " << n_cell_iv << endl;
	if(!IsCellValid.iu_iv_valid(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize))  continue;
	CellCentreXY = TheCell.GetCellCentreCoordinatesForPlots(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize);
	double iux = (CellCentreXY.first < 0 ) ? (CellCentreXY.first + 0.0001) : (CellCentreXY.first - 0.0001) ;
	double iyy = (CellCentreXY.second < 0 ) ? (CellCentreXY.second + 0.0001) : (CellCentreXY.second - 0.0001);
	int nsample = 0;
 
	if((iux <= -0.25 && iux >= -3.25) && (iyy <= -0.25 && iyy>= -5.25 )){
	  Cell_counter1++;
	  Average_Pedestal_Per_Event1 += SKI_1[nsample].adc();
	  Sum_Hist_cells_SKI1[event.id().event() - 1]->Fill(SKI_1[nsample].adc()); 
	}

	if((iux >= 0.25 && iux <= 3.25) && (iyy <= -0.25 && iyy>= -5.25 )){
	  Cell_counter2++;
	  Average_Pedestal_Per_Event2 += SKI_1[nsample].adc();
	}

      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //
      for(SKIROC2DigiCollection::const_iterator j = Coll.begin(); j != Coll.end(); j++) {
	int flag_calib = 0;
	const SKIROC2DataFrame& SKI = *j ;
	int n_layer = (SKI.detid()).layer();
	int n_sensor_IU = (SKI.detid()).sensorIU();
	int n_sensor_IV = (SKI.detid()).sensorIV();
	int n_cell_iu = (SKI.detid()).iu();
	int n_cell_iv = (SKI.detid()).iv(); 
	std::pair<int,int> uvpair (n_cell_iu, n_cell_iv);

	if((n_cell_iu == -1 && n_cell_iv == 2) || (n_cell_iu == 2 && n_cell_iv == -4)) flag_calib =1; 
	if(DEBUG) cout << endl << " Layer = " << n_layer << " Sensor IU = " << n_sensor_IU << " Sensor IV = " << n_sensor_IV << " Cell iu = " << n_cell_iu << " Cell iu = " << n_cell_iv << endl;
	if(!IsCellValid.iu_iv_valid(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize))  continue;
	CellCentreXY = TheCell.GetCellCentreCoordinatesForPlots(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize);
	double iux = (CellCentreXY.first < 0 ) ? (CellCentreXY.first + 0.0001) : (CellCentreXY.first - 0.0001) ;
	double iyy = (CellCentreXY.second < 0 ) ? (CellCentreXY.second + 0.0001) : (CellCentreXY.second - 0.0001);
	int nsample = 0;

	//map of iu,iv --> X,Y
	XYMAP[uvpair] = pair<float,float>(iux,iyy);

	//Get cell Type
	int nCellVertex = TheCell.GetCellCoordinatesForPlots(n_layer, n_sensor_IU, n_sensor_IV, n_cell_iu, n_cell_iv, sensorsize).size();
	if(nCellVertex == 4 ) cellTypeCode[channelIndex] = 1; // half cells
	else if(nCellVertex == 6 ) { 
	  if(flag_calib == 1) cellTypeCode[channelIndex] = 2; // calibration cell 
	  else cellTypeCode[channelIndex] = 3;  
	} else cellTypeCode[channelIndex] = 0; // unknown
			

	//Fill ADC values to channels
	if(flag_calib == 1){
	  if((iux <= 0.25 && iyy>= -0.25 ) || (iux < -0.5) ) {	
	    ADCData[channelIndex] = 0.5*(SKI[nsample].adc() - (Average_Pedestal_Per_Event1/(Cell_counter1)));
	    XData[channelIndex] = iux;
	    YData[channelIndex] = iyy;
	    UData[channelIndex] = n_cell_iu;
	    VData[channelIndex] = n_cell_iv;
	    channelNum[channelIndex] = channelIndex;
	    ADCMAP [uvpair] = ADCData[channelIndex];
	  }
	  else if((iux > -0.25 && iyy < -0.50 ) || (iux > 0.50)) {
	    ADCData[channelIndex] = 0.5*(SKI[nsample].adc() - (Average_Pedestal_Per_Event2/(Cell_counter2)));
	    XData[channelIndex] = iux;
	    YData[channelIndex] = iyy;	
	    UData[channelIndex] = n_cell_iu;
	    VData[channelIndex] = n_cell_iv;	    
	    channelNum[channelIndex] = channelIndex;
	    ADCMAP [uvpair] = ADCData[channelIndex];			  
	  }					    
	}
	else{
	  if((iux <= 0.25 && iyy>= -0.25 ) || (iux < -0.5) ) {
	    ADCData[channelIndex] = (SKI[nsample].adc() - (Average_Pedestal_Per_Event1/(Cell_counter1)));
	    XData[channelIndex] = iux;
	    YData[channelIndex] = iyy;
	    UData[channelIndex] = n_cell_iu;
	    VData[channelIndex] = n_cell_iv;	    
	    channelNum[channelIndex] = channelIndex;
	    ADCMAP [uvpair] = ADCData[channelIndex];	   
	  }
	  else if((iux > -0.25 && iyy < -0.50 ) || (iux > 0.50)) {
	    ADCData[channelIndex] = (SKI[nsample].adc() - (Average_Pedestal_Per_Event2/(Cell_counter2)));
	    XData[channelIndex] = iux;
	    YData[channelIndex] = iyy;
	    UData[channelIndex] = n_cell_iu;
	    VData[channelIndex] = n_cell_iv;	    
	    channelNum[channelIndex] = channelIndex;
	    ADCMAP [uvpair] = ADCData[channelIndex];	    
	  }					   
	}
		
	//High Gain ADC stuff. This is not properly done yet.
	nsample = 1;								
	ADCHighData[channelIndex] = SKI[nsample-1].tdc() - Sum_Hist_cells_SKI1[event.id().event() - 1]->GetMean();			       
				
	channelIndex ++;
      }
      ped1 = (Average_Pedestal_Per_Event1/(Cell_counter1));
      ped2 = (Average_Pedestal_Per_Event2/(Cell_counter2));			
    }
  } else {
    edm::LogWarning("DQM") << "No SKIROC2 Digis";
  }


  float max = -999.;
  float tmpX = -999.;
  float tmpY = -999.;
  // int tmpU = -9;
  // int tmpV = -9;
  for (int ii = 0; ii<128; ii++)
    {
      if(ADCData[ii] > max) 
	{
	  maxChannelIndex = ii;
	  max = ADCData[ii];
	  tmpX = XData[ii];
	  tmpY = YData[ii];
	  // tmpU = UData[ii];
	  // tmpV = VData[ii];
	}
    }

  maxADC = max;
  maxX   = tmpX;
  maxY   = tmpY;	

  //***********************************************************
  //Add energy of cluster around the max energy cell
  //***********************************************************

  for (int ii = 0; ii<128; ii++) {

    //include only good cells
    if (cellTypeCode[ii] != 3) continue;

    //first circle around the max
    if (sqrt( pow(XData[ii] - maxX,2) + pow(YData[ii]-maxY,2)) < 1.2) {
      // std::cout << ii << " : " << UData[ii] << " : " << VData[ii] << " : "
      // 		 << XData[ii] - maxX << " , " << YData[ii]-maxY << " : " 
      // 		 << sqrt( pow(XData[ii] - maxX,2) + pow(YData[ii]-maxY,2)) << " : "
      // 		 << ADCData[ii] << "\n";
      ADCSum1 += ADCData[ii];
    }
    
    //2nd circle around the max
    if (sqrt( pow(XData[ii] - maxX,2) + pow(YData[ii]-maxY,2)) < 2.3) {
      // std::cout << ii << " : " << UData[ii] << " : " << VData[ii] << " : "
      // 		<< XData[ii] - maxX << " , " << YData[ii]-maxY << " : " 
      // 		<< sqrt( pow(XData[ii] - maxX,2) + pow(YData[ii]-maxY,2)) << " : "
      // 		<< ADCData[ii] << "\n";
      ADCSum2 += ADCData[ii];
    }            
  }
  //std::cout << "ADCSUM " << ADCSum1 << "\n";


  //***********************************************************
  //Compute Centroid of the Shower
  //***********************************************************
  double tmpCentroidX = 0;
  double tmpCentroidY = 0;

  for (int ii = 0; ii<128; ii++) {

    //include only good cells
    if (cellTypeCode[ii] != 3) continue;

    //first circle around the max
    if (sqrt( pow(XData[ii] - maxX,2) + pow(YData[ii]-maxY,2)) < 1.2) {
      tmpCentroidX += XData[ii]*ADCData[ii];
      tmpCentroidY += YData[ii]*ADCData[ii];
    }
       
  }
  centroidX = tmpCentroidX / ADCSum1;
  centroidY = tmpCentroidY / ADCSum1;



  //fill ntuple here
  outputTree->Fill();

}//analyze method ends here


// ------------ method called once each job just before starting event loop  ------------
void
DigiNtupler::beginJob()
{
  outputTree->Branch("event", &eventNum,"event/i");
  outputTree->Branch("X", XData,"X[128]/F"); 
  outputTree->Branch("Y", YData,"Y[128]/F"); 
  outputTree->Branch("U", UData,"U[128]/F"); 
  outputTree->Branch("V", VData,"V[128]/F"); 
  outputTree->Branch("ADC", ADCData,"ADC[128]/F"); 
  outputTree->Branch("ADCHigh", ADCHighData,"ADCHigh[128]/F");
  outputTree->Branch("maxChannelIndex", &maxChannelIndex,"maxChannelIndex/i"); 
  outputTree->Branch("maxADC", &maxADC,"maxADC/F"); 
  outputTree->Branch("ADCSum1", &ADCSum1,"ADCSum1/F"); 
  outputTree->Branch("ADCSum2", &ADCSum2,"ADCSum2/F"); 
  outputTree->Branch("centroidX", &centroidX,"centroidX/F"); 
  outputTree->Branch("centroidY", &centroidY,"centroidY/F"); 
  outputTree->Branch("maxX", &maxX,"maxX/F"); 
  outputTree->Branch("maxY", &maxY,"maxY/F"); 
  outputTree->Branch("ped1", &ped1,"ped1/F"); 
  outputTree->Branch("ped2", &ped2,"ped2/F"); 
  outputTree->Branch("channelNum", channelNum,"channelNum[128]/I"); 
  outputTree->Branch("cellTypeCode", cellTypeCode,"cellTypeCode[128]/i"); 
}

// ------------ method called once each job just after ending the event loop  ------------
void
DigiNtupler::endJob()
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
DigiNtupler::fillDescriptions(edm::ConfigurationDescriptions& descriptions)
{
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(DigiNtupler);