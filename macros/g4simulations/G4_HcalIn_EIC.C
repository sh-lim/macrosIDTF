//Inner HCal construction macro

//EIC modification: Make support ring in h-going side smaller (larger inner ring diameter) to
//give room for PID detectors

//Inner HCal absorber material selector:
//false - Default, absorber material is SS310
//true - Choose if you want Aluminum
const bool inner_hcal_material_Al = false;


enum enu_HCalIn_clusterizer
  {
    kHCalInGraphClusterizer,

    kHCalInTemplateClusterizer
  };

//! template clusterizer, RawClusterBuilderTemplate, as developed by Sasha Bazilevsky
enu_HCalIn_clusterizer HCalIn_clusterizer = kHCalInTemplateClusterizer;
//! graph clusterizer, RawClusterBuilderGraph
//enu_HCalIn_clusterizer HCalIn_clusterizer = kHCalInGraphClusterizer;


// Init is called by G4Setup.C
void HCalInnerInit() {}

double HCalInner(PHG4Reco* g4Reco,
                 double radius,
                 const int crossings,
                 const int absorberactive = 0,
                 int verbosity = 0) {
  // all sizes are in cm!

  gSystem->Load("libg4detectors.so");
  gSystem->Load("libg4testbench.so");


  PHG4InnerHcalSubsystem *hcal = new PHG4InnerHcalSubsystem("HCALIN");
  // these are the parameters you can change with their default settings
  // hcal->set_string_param("material","SS310");
  if(inner_hcal_material_Al)
    {
      cout <<"HCalInner - construct inner HCal absorber with G4_Al"<<endl;
      hcal->set_string_param("material","G4_Al");
    }

  hcal->SetActive();
  hcal->SuperDetector("HCALIN");
  if (absorberactive)
    {
      hcal->SetAbsorberActive();
    }
  hcal->OverlapCheck(overlapcheck);

  g4Reco->registerSubsystem( hcal );

  radius = hcal->get_double_param("outer_radius");

  HCalInner_SupportRing(g4Reco,absorberactive);

  radius += no_overlapp;
  return radius;
}

//! A rough version of the inner HCal support ring, from Richie's CAD drawing. - Jin
void HCalInner_SupportRing(PHG4Reco* g4Reco,
                           const int absorberactive = 0) {

  gSystem->Load("libg4detectors.so");
  gSystem->Load("libg4testbench.so");

  const double z_ring1 = (2025 + 2050) / 2. / 10.;
  const double z_ring2 = (2150 + 2175) / 2. / 10.;
  const double dz = 25. / 10.;
  const double innerradius_e = 116.;
  const double innerradius_h = 138.;
  const double maxradius = 178.0 - 0.001; // avoid touching the outer HCal envelop volumne

  const double z_rings[] =
    { -z_ring2, -z_ring1, z_ring1, z_ring2, 0, 0, 0, 0 };

  PHG4CylinderSubsystem *cyl;

  for (int i = 0; i < 4; i++)
    {
      double innerradius = innerradius_e;
      if ( z_rings[i] > 0 )
        innerradius = innerradius_h;

      cyl = new PHG4CylinderSubsystem("HCALIN_SPT_N1", i);
      cyl->set_double_param("place_z",z_rings[i]);
      cyl->SuperDetector("HCALIN_SPT");
      cyl->set_double_param("radius",innerradius);
      cyl->set_int_param("lengthviarapidity",0);
      cyl->set_double_param("length",dz);
      cyl->set_string_param("material","SS310");
      cyl->set_double_param("thickness",maxradius - innerradius);
      if (absorberactive)
        {
          cyl->SetActive();
        }
      g4Reco->registerSubsystem(cyl);
    }

  return;
}


void HCALInner_Cells(int verbosity = 0) {

  gSystem->Load("libfun4all.so");
  gSystem->Load("libg4detectors.so");
  Fun4AllServer *se = Fun4AllServer::instance();

  PHG4HcalCellReco *hc = new PHG4HcalCellReco("HCALIN_CELLRECO");
  hc->Detector("HCALIN");
  //  hc->Verbosity(2);
  // check for energy conservation - needs modified "infinite" timing cuts
  // 0-999999999
  //  hc->checkenergy();
  // timing cuts with their default settings
  // hc->set_double_param("tmin",0.);
  // hc->set_double_param("tmax",60.0);
  // or all at once:
  // hc->set_timing_window(0.0,60.0);
  se->registerSubsystem(hc);

  return;
}

void HCALInner_Towers(int verbosity = 0) {

  gSystem->Load("libg4calo.so");
  gSystem->Load("libcalo_reco.so");
  Fun4AllServer *se = Fun4AllServer::instance();

  HcalRawTowerBuilder *TowerBuilder = new HcalRawTowerBuilder("HcalInRawTowerBuilder");
  TowerBuilder->Detector("HCALIN");
  TowerBuilder->set_sim_tower_node_prefix("SIM");
  TowerBuilder->Verbosity(verbosity);
  se->registerSubsystem( TowerBuilder );

  // From 2016 Test beam sim
  RawTowerDigitizer *TowerDigitizer = new RawTowerDigitizer("HcalInRawTowerDigitizer");
  TowerDigitizer->Detector("HCALIN");
  //  TowerDigitizer->set_raw_tower_node_prefix("RAW_LG");
  TowerDigitizer->set_digi_algorithm(
                                     RawTowerDigitizer::kSimple_photon_digitalization);
  TowerDigitizer->set_pedstal_central_ADC(0);
  TowerDigitizer->set_pedstal_width_ADC(1); // From Jin's guess. No EMCal High Gain data yet! TODO: update
  TowerDigitizer->set_photonelec_ADC(32. / 5.);
  TowerDigitizer->set_photonelec_yield_visible_GeV(32. / 5 / (0.4e-3));
  TowerDigitizer->set_zero_suppression_ADC(-0); // no-zero suppression
  se->registerSubsystem(TowerDigitizer);

  //Default sampling fraction for SS310
  double visible_sample_fraction_HCALIN = 0.0631283 ; //, /gpfs/mnt/gpfs04/sphenix/user/jinhuang/prod_analysis/hadron_shower_res_nightly/./G4Hits_sPHENIX_pi-_eta0_16GeV-0000.root_qa.rootQA_Draw_HCALIN_G4Hit.pdf

  if(inner_hcal_material_Al) visible_sample_fraction_HCALIN = 0.162166; //for "G4_Al", Abhisek Sen <sen.abhisek@gmail.com>

  RawTowerCalibration *TowerCalibration = new RawTowerCalibration("HcalInRawTowerCalibration");
  TowerCalibration->Detector("HCALIN");
  //  TowerCalibration->set_raw_tower_node_prefix("RAW_LG");
  //  TowerCalibration->set_calib_tower_node_prefix("CALIB_LG");
  TowerCalibration->set_calib_algorithm(RawTowerCalibration::kSimple_linear_calibration);
  TowerCalibration->set_calib_const_GeV_ADC(0.4e-3 / visible_sample_fraction_HCALIN);
  TowerCalibration->set_pedstal_ADC(0);
  TowerCalibration->set_zero_suppression_GeV(-1); // no-zero suppression
  se->registerSubsystem(TowerCalibration);

  return;
}

void HCALInner_Clusters(int verbosity = 0) {
  gSystem->Load("libcalo_reco.so");

  Fun4AllServer *se = Fun4AllServer::instance();

  if (HCalIn_clusterizer == kHCalInTemplateClusterizer)
    {
      RawClusterBuilderTemplate* ClusterBuilder = new RawClusterBuilderTemplate("HcalInRawClusterBuilderTemplate");
      ClusterBuilder->Detector("HCALIN");
      ClusterBuilder->Verbosity(verbosity);
      se->registerSubsystem( ClusterBuilder );

    }
  else if (HCalIn_clusterizer == kHCalInGraphClusterizer)
    {
      RawClusterBuilderGraph* ClusterBuilder = new RawClusterBuilderGraph("HcalInRawClusterBuilderGraph");
      ClusterBuilder->Detector("HCALIN");
      ClusterBuilder->Verbosity(verbosity);
      se->registerSubsystem( ClusterBuilder );

    }
  else
    {
      cout <<"HCalIn_Clusters - unknown clusterizer setting!"<<endl;
      exit(1);
    }
  return;
}

void HCALInner_Eval(std::string outputfile, int verbosity = 0) {

  gSystem->Load("libfun4all.so");
  gSystem->Load("libg4eval.so");
  Fun4AllServer *se = Fun4AllServer::instance();

  CaloEvaluator* eval = new CaloEvaluator("HCALINEVALUATOR", "HCALIN", outputfile.c_str());
  eval->Verbosity(verbosity);
  se->registerSubsystem( eval );

  return;
}
