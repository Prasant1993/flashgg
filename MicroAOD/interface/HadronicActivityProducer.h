#ifndef __HadronicActivityProducer_h__
#define __HadronicActivityProducer_h__

#include <string>
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/ValueMap.h"

#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/Event.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/PatCandidates/interface/libminifloat.h"

#include "CommonTools/CandUtils/interface/AddFourMomenta.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CompositePtrCandidate.h"
#include "DataFormats/Candidate/interface/CompositeCandidate.h"

#include "flashgg/DataFormats/interface/DiPhotonCandidate.h"

namespace flashgg { 
	template<class T> struct TrivialVetex { size_t operator()(const T& obj) { return 0; } };
	
    template <class T, class V=TrivialVetex<typename T::value_type> >
    class HadronicActivityProducer : public edm::global::EDProducer<> {
        
    public:
        explicit HadronicActivityProducer(const edm::ParameterSet&);
        ~HadronicActivityProducer();
        
        virtual void produce(edm::StreamID, edm::Event&, const edm::EventSetup&) const;
        
    private:
        std::vector<edm::EDGetTokenT<edm::View<reco::Candidate> > > srcTokens_;
        int max_;
        bool veto_;
        double vetocone_;
        edm::EDGetTokenT<T> vetoToken_;
    };
    
    template <class T, class V>
    HadronicActivityProducer<T,V>::HadronicActivityProducer(const edm::ParameterSet& iConfig) : 
        max_(-1),
        veto_( iConfig.exists("veto") ),
        vetocone_( 0.4 )
    {
        if( iConfig.exists("maxCand") ) {
            max_ = iConfig.getParameter<int>("maxCand");
        }

        std::vector<edm::InputTag> srcTags;
        if( iConfig.existsAs<edm::InputTag>("src") ) { 
            srcTags.push_back(iConfig.getParameter<edm::InputTag> ( "src" ) ); 
        } else { 
            srcTags = iConfig.getParameter<std::vector<edm::InputTag> > ( "src" ); 
        }
        for( auto & tag : srcTags ) { srcTokens_.push_back( consumes<edm::View<reco::Candidate> >( tag ) ); }

        if( veto_ ) { vetoToken_ = consumes<T>( iConfig.getParameter<edm::InputTag> ( "veto" ) ); }
        if( iConfig.exists("vetocone") ) { vetocone_ = iConfig.getParameter<double>("vetocone"); }

        produces<std::vector<reco::CompositeCandidate> >();	
    }
    
    template <class T, class V>
    HadronicActivityProducer<T,V>::~HadronicActivityProducer() {}
    
    template <class T, class V>
    void HadronicActivityProducer<T,V>::produce(edm::StreamID, edm::Event& iEvent, const edm::EventSetup& iSetup) const {
        
        std::unique_ptr<std::vector<reco::CompositeCandidate> > outPtr(new std::vector<reco::CompositeCandidate>(1));
        
        auto & out = outPtr->at(0);
        
        edm::Handle<T> veto;
        size_t index = 0;
        if( veto_ )  {
            iEvent.getByToken( vetoToken_,  veto);
            if( veto->size() > 0 ) index = V()(veto->at(0));
        }
        if( index > srcTokens_.size()-1 ) { index = 0; }
        
        edm::Handle<edm::View<reco::Candidate> > src;
        iEvent.getByToken( srcTokens_[index],  src);
        auto & collection = *src;
        
        int count = ( max_ > 0 ? max_ : collection.size() );
        for( size_t iob = 0; iob<collection.size() && count > 0; ++iob ) {
            auto & cand = collection.at(iob);
            bool add = true;
            if( ( veto_ && veto->size() > 0 ) &&
                ( reco::deltaR(*(veto->at(0).leadingPhoton()),cand) < vetocone_ || reco::deltaR(*(veto->at(0).subLeadingPhoton()),cand) < vetocone_ ) ) { add=false; }
            if( add ) { out.addDaughter(cand); --count; }
        }
        
        AddFourMomenta addP4;
        addP4.set(out);
        
        iEvent.put(std::move(outPtr));
    }
}

/// #include "FWCore/Framework/interface/MakerMacros.h"
/// DEFINE_FWK_MODULE(HadronicActivityProducer);

#endif // __HadronicActivityProducer_h__
// Local Variables:
// mode:c++
// indent-tabs-mode:nil
// tab-width:4
// c-basic-offset:4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

