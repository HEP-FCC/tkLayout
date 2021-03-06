/**
 * Track.h
 *
 * Created on: 20. 4. 2016
 *     Author: Drasal (CERN)
 */

#ifndef INCLUDE_TRACK_H_
#define INCLUDE_TRACK_H_

#include <cmath>
#include <memory>
#include <set>
#include <vector>

#include "global_constants.h"
#include "DetectorModule.h"
#include "Hit.h"
#include <Math/Vector3D.h>
#include <TMatrixT.h>
#include <TMatrixTSym.h>

// Forward declaration
class Track;
class RILength;

#undef TRACK_DEBUG_RZ

//Typedef
typedef std::unique_ptr<Track> TrackPtr;
typedef std::vector<TrackPtr>  TrackCollection;

/**
 * @class Track
 * @details The Track class is essentially a collection of consecutive hits used to calculate track parameters
 * together with their correlation matrix. These hits can bet set as active (used in tracking) or inactive (unused in
 * tracking). The track model assumes mag. field to be variable only in z: B = B(z).e_z + 0.e_x + 0.e_y, further
 * the parabolic approximation for track is considered (a la Karimaki parametrization + Multiple Coulomb scattering
 * is taken into account), disentangling the problem to fit 5 track parameters simultaneously into a problem of
 * fitting track independenly in R-Phi and s-Z plane. The direction, in which Multiple scattering is calculated is
 * defined by particle direction or track propagation requirements. Either inside-out or outside-in can be set. The final
 * covariance matrix is 3x3 (rho, phi0, d0) + 2x2 (cotg(theta), z0). The parametrization used is the following, one fits:
 * phi0, 1/R, impact parameter in R-Phi plane d0 & cotg(theta), impact parameter in s-Z plane z0. To provide tracking independently
 * on complexity of defined tracker geometry, each track is assigned a tag, all sub-trackers being assigned the same tag
 * are then used in the track for tracking. In order to extract individual track parameters at arbitrary reference
 * point (not only at [0,0,0]), the standard technique of covariance propagator is applied (for details see getDelta***()
 * methods). The propagators are currently implemented for const Bz, not for case of B = B(z).
 */
class Track {

public:

  //! Track constructor -> need to use setter methods to set: 2 of these [theta, phi, eta, cot(theta)] & 2 of these [mag. field, transv. momentum, radius]
  Track();

  //! Track copy-constructor -> creates deep copy of hit vector
  Track(const Track& track);

  //! Assign operator with deep copy of hit vector
  Track& operator=(const Track &track);

  //! Destructor
  ~Track();

  //
  // Define track properties

  //! Add new hit to track and return a pointer to that hit (+ prune hits + resort hits)
  void addHit(HitPtr newHit);

  //! Add IP constraint (+ prune hits + resort hits) to the track, technically new hit is assigned: with no material and hit resolution in R-Phi as dr, in s-Z as dz
  void addIPConstraint(double dr, double dz);

  //! Set track polar angle - theta, azimuthal angle - phi, particle transverse momentum - pt (signed: + -> particle in-out, - -> particle out-in)
  const ROOT::Math::Polar3DVector& setThetaPhiPt(const double& newTheta, const double& newPhi, const double& newPt);

  //! Set track origin
  const XYZVector& setOrigin(const double& X, const double& Y, const double& Z) {m_origin.SetCoordinates(X,Y,Z); return m_origin;}

  //! Re-set transverse momentum + resort hits (if changing direction) + initiate recalc of cov matrices + prune hits (otherwise they may not lie on the new track, originally found at high pT limit)
  void resetPt(double newPt);

  //! Set active only trigger hits, so all other hits are made as inactive
  void keepTriggerHitsOnly();

  //! Set active only hits with the given tag. If tag="all" all hits coming from measurement planes or IP will be set as active
  void keepTaggedHitsOnly(const string& tag, bool useIP = true);

  //! Remove material from all assigned hits -> modify all hits such as they are without any material
  void removeMaterial();

  //
  // Print methods

  //! Helper method printing track covariance matrices in R-Phi
  void printErrors();

  //! Helper method printing symmetric matrix
  void printSymMatrix(const TMatrixTSym<double>&) const;

  //! Helper method printing general matrix
  void printMatrix(const TMatrixT<double>&) const;

  //! Helper method printing track hits
  void printHits() const;

  //! Helper method printing active track hits
  void printActiveHits() const;

  //! Does track contain no hits?
  bool hasNoHits() const {return m_hits.empty(); }

  //
  // Getter methods
  double getTheta() const              { return m_theta;}
  double getEta() const                { return m_eta; }
  double getCotgTheta() const          { return m_cotgTheta; }
  double getPhi() const                { return m_phi;}
  double getTransverseMomentum() const { return m_pt; }
  double getPt() const                 { return m_pt; }
  double getP() const                  { return m_p; }
  
  //! Get DeltaRho (error on 1/R) at refPoint [rPos, zPos] in "parabolic" approximation.
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  //! Using 3x3 covariance propagator in case [r,z]!=[0,0]
  double getDeltaRho(double refPointRPos, bool propagOutIn=true);

  //! Get DeltaPtOvePt at refPoint [rPos, zPos] in "parabolic" approximation (utilize the calculated deltaRho quantity).
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  double getDeltaPtOverPt(double refPointRPos, bool propagOutIn=true);

  //! Get DeltaPtOverPt at [0,0] in full math. approach, i.e. using full 5x5 covariance matrix
  double getDeltaPtOverPtFull();

  //! Get DeltaPOverP at refPoint [rPos, zPos] in "parabolic" approximation (utilize deltaRho & deltaCotgTheta quantities)
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  double getDeltaPOverP(double refPointRPos, bool propagOutIn=true);

  //! Get DeltaPhi(Phi0) at refPoint [rPos, zPos] ([0,0]) in "parabolic" approximation
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  //! Using 3x3 covariance propagator in case [r,z]!=[0,0]
  double getDeltaPhi(double refPointRPos, bool propagOutIn=true);
  double getDeltaPhi0() { return getDeltaPhi(0.0); };

  //! GetDeltaPhi at [0,0] in full math. approach, i.e. using full 5x5 covariance matrix
  double getDeltaPhi0Full();

  //! Get DeltaD (D0) at refPoint [rPos, zPos] ([0,0]) in "parabolic" approximation
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  //! Using 3x3 covariance propagator in case [r,z]!=[0,0]
  double getDeltaD(double refPointRPos, bool propagOutIn=true);
  double getDeltaD0() { return getDeltaD(0.0); };

  //! Get DeltaD0, i.e. at [0,0] in full math. approach, i.e. using full 5x5 covariance matrix
  double getDeltaD0Full();

  //! Get DeltaCtgTheta at refPoint [rPos, zPos] in "parabolic" approximation
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  //! Using 2x2 covariance propagator in case [r,z]!=[0,0]
  double getDeltaCtgTheta(double refPointRPos, bool propagOutIn=true);

  //! Get DeltaCtgTheta at [0,0] in full math. approach, i.e. using fulll 5x5 covariance matrix
  double getDeltaCtgThetaFull();

  //! Get DeltaZ (Z0) at refPoint [rPos, zPos] ([0,0]) in "parabolic" approximation
  //! Propagator direction defines, which part of tracker (at higher radii or lower radii from the ref. point) is going to be used.
  //! Using 2x2 covariance propagator in case [r,z]!=[0,0]
  double getDeltaZ(double refPointRPos, bool propagOutIn=true);
  double getDeltaZ0() { return getDeltaZ(0.0); }

  //! Get DeltaZ0, i.e. at [0,0] in full math. approach, i.e. using full 5x5 covariance matrix
  double getDeltaZ0Full();

  //! Get DeltaT0 at refPoint [rPos, zPos] ([0,0]) combining all time-stamps along the track. Use track path length, defined as
  //! difference between time layer and ref. point to calculated the time of flight correction factor to individual time measurements.
  double getDeltaT(double refPointRPos);
  double getDeltaT0() { return getDeltaT(0.0); }

  //! Get DeltaCTau for secondary particles coming from the primary vertex at ~ [0,0] -> an important quantity to estimate the
  //! resolution of secondary vertices
  double getDeltaCTau();

  // Calculate magnetic field at given z, assuming B = B(z).e_z + 0.e_x + 0 e_y
  double getMagField(double zPos) const;

  // Calculate radius or 1/R at given z, assuming B = B(z).e_z + 0.e_x + 0 e_y
  double getRho(double zPos) const    { return (getRadius(zPos)!=0 ? 1/getRadius(zPos) : 0);}
  double getRadius(double zPos) const { return fabs(m_pt / (c_K * getMagField(zPos))); }

  const ROOT::Math::Polar3DVector& getDirection() const { return m_direction; }
  const XYZVector&                 getOrigin() const    { return m_origin; }

  //! Get number of active hits assigned to track for given tag: pixel, strip, tracker, etc. (as defined in the geometry config file). If tag specified as "all" no extra tag required
  int getNActiveHits(std::string tag, bool useIP = true) const;

  //! Get number of hits, set as active & coming from measurement planes or IP constraint. All hits must be assigned to tracker with given tag. If tag specified as "all", all module & IP hits assigned.
  int getNMeasuredHits(std::string tag, bool useIP = true) const;

  //! Get reference to a hit, which can be measured, i.e. coming from measurement plane (active or inactive) or IP constraint
  const Hit* getMeasurableOrIPHit(int iHit);

  //! Reverse search -> Get reversely reference to a hit, which can be measured, i.e. coming from measurement plane (active or inactive) or IP constraint
  const Hit* getRMeasurableOrIPHit(int iHit);

  //! Get the probabilty of having "clean" hits for nuclear-interacting particles for given tag: pixel, strip, tracker, etc. (as defined in the geometry config file)
  //! If tag specified as "all" no extra tag required
  std::vector<double> getHadronActiveHitsProbability(std::string tag);

  //! Get the probabilty of having a given number of "clean" hits for nuclear-interacting particles for given tag: pixel, strip, tracker, etc. (as defined in the geometry config file)
  double getHadronActiveHitsProbability(std::string tag, int nHits);

  //! Get number of hits assigned
  int getNHits() const { return m_hits.size(); }

  //! Hits collection iterators
  HitCollection::const_iterator getBeginHits() { return m_hits.cbegin(); }
  HitCollection::const_iterator getEndHits()   { return m_hits.cend(); }

  //! Get track material
  RILength getMaterial() const;

  //! Get all tags assigned to the track (see m_tags variable below
  const std::set<std::string>& getTags() const { return m_tags; }

  //! Get a vector of pairs: Detector module & hit type for Trigger hits
  std::vector<std::pair<const DetectorModule*, HitModuleType>> getHitModules() const;

//  void addEfficiency(double efficiency, bool alsoPixel = false);
//  void keepTriggerOnly();
//  void setTriggerResolution(bool isTrigger);
//  // static bool debugRemoval; // debug
//  double expectedTriggerPoints(const double& triggerMomentum) const;
//#ifdef TRACK_DEBUG_RZ
//  static bool debugRZCovarianceMatrix;  // debug
//  static bool debugRZCorrelationMatrix;  // debug
//  static bool debugRZErrorPropagation;  // debug
//#endif

protected:

  //! Main method calculating track parameters in both r-phi & s-z planes, using linear fit with parameters: 1/R, phi0, d0, cotg(theta) & z0. Within the calculation an NxN variance
  //! matrix is used (N hits = K+L: K active hits on detectors + L passive (artificial) hits due to material). Ref. point dictates, whether Multiple scattering effects need to be
  //! calculated inside-out or outside-in. MS effect is symmetric as regards track fitting. PropagOutIn variable defines whether detectors at higher R than the ref. point (true)
  //! should be used for error calculation/propagation (e.g. d0,z0) or whether detectors at lower R (false). E.g. for standard estimation of D0,Z0 parameters one calculates MS
  //! inside->out from rPos=0 (zPos can be calculated from rPos using theta).
  //! Return true if covariance matrix correctly calculated
  bool computeCovarianceMatrix(double refPoint, bool propagOutIn);

  //! Helper fce returning derivative: d[f(rho, d0, phi0)]/d[rho], where f approximates
  //! a helix by set of parabolas. In general, N connected parabolas used, for const B
  //! field only one parabola assumed.
  double computeDfOverDRho(double rPos, double zPos);

  //! Sort internally all hits assigned to this track -> sorting algorithm based on hit radius - by smaller radius sooner or vice-versa (inner-to-outer approach or vice-versa)
  void sortHits(bool bySmallerR);

  //! Remove hits, which would be at radius higher than curling_radius (hits assumed to be found in line approach, which is perfectly valid if the geometry has a cylindrical
  //! symmetry, but then all hits above curling radius must be removed -> check that hits are below curling radius). Factor f stands for a cut_off factor to avoid passage of
  //! modules at very shallow angle, i.e. hit radius close to the curling_radius: defined by max sin(phi-phi0), where phi is set as follows: arc_length = R*2*phi
  bool isHitRPosLowerThanCurlingRadius(double rPos, double zPos) { return rPos<track_maxSinPhi*2*getRadius(zPos); }
  bool pruneHits();

  const double c_K=0.299792458;//!< Track pT[GeV/c] = K . Bz[T] . R[m]

  double m_theta;              //!< Track shot at given theta & phi, i.e. theta at primary vertex
  double m_phi;                //!< Track shot at given theta & phi, i.e. phi at primary vertex
  double m_pt;                 //!< Particle transverse momentum (assuming B = fce of z only -> pT doesn't change along the path, only radius changes), pT sign: + -> particle traverses inside-out, - -> particle traverses outside-in
  double m_p;                  //!< Particle momentum
  double m_cotgTheta;          //!< Automatically calculated from theta at [0,0]
  double m_eta;                //!< Automatically calculated from theta at [0,0]

  ROOT::Math::Polar3DVector  m_direction; //!< Track parameters as a 3-vector: R, theta, phi
  XYZVector                  m_origin;    //!< Track origin as a 3-vector: X, Y, Z TODO: For tracking model origin assumed to be at [0,0,0]

  bool   m_reSortHits;         //!< Caching whether necessary to resort hits (sorting will be done again if a new hit added or direction changed)
  bool   m_covDone;            //!< Caching whether errors in R-Phi & s-z already calculated (will be recalculated, if direction of propagation changed, or added new hit etc.)
  bool   m_refPointRPosCache;  //!< Caching the last r position of ref. point, which was used to calculate track errors (if changed -> recalculate track parameters).
  bool   m_propagOutInCache;   //!< Caching the last used propagator direction (if changed -> recalculate track parameters).

  HitCollection         m_hits;         //!< Position hits assigned to track // TODO: Combine time & pos. hits into 1 vector
  HitCollection         m_timeHits;     //!< Time stamps assigned to track
  std::set<std::string> m_tags;         //!< Which subdetectors to be used in tracking (each subdetector is tagged by a set of tags, e.g. pixel, fwd, tracker -> used in tracking of pixels, fwd tracking & full tracker)

  // Track parameters covariance matrices
  TMatrixT<double>      m_covMatrixRPhi; //!< 3x3 Covariance matrix in R-Phi: pT (1/R), phi, d0 at ref. point [r,z] = [0,0]: C(3x3)
  TMatrixT<double>      m_covMatrixRZ;   //!< 2x2 Covariance matrix in R-Phi: cotg(theta), z0 at ref. point [r,z] = [0,0]: C(2x2)
  TMatrixT<double>      m_covMatrixFull; //!< Full 5x5 Covariance matrix: pT (1/R), phi, d0, cotg(theta), z0 at ref. point [r,z] = [0,0]: C(5x5)

}; // Class

#endif /* INCLUDE_TRACK_H_ */
