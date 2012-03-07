//==- ScheduleDAGInstrs.h - MachineInstr Scheduling --------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ScheduleDAGInstrs class, which implements
// scheduling for a MachineInstr-based dependency graph.
//
//===----------------------------------------------------------------------===//

#ifndef SCHEDULEDAGINSTRS_H
#define SCHEDULEDAGINSTRS_H

#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SparseSet.h"
#include <map>

namespace llvm {
  class MachineLoopInfo;
  class MachineDominatorTree;
  class LiveIntervals;

  /// LoopDependencies - This class analyzes loop-oriented register
  /// dependencies, which are used to guide scheduling decisions.
  /// For example, loop induction variable increments should be
  /// scheduled as soon as possible after the variable's last use.
  ///
  class LLVM_LIBRARY_VISIBILITY LoopDependencies {
    const MachineLoopInfo &MLI;
    const MachineDominatorTree &MDT;

  public:
    typedef std::map<unsigned, std::pair<const MachineOperand *, unsigned> >
      LoopDeps;
    LoopDeps Deps;

    LoopDependencies(const MachineLoopInfo &mli,
                     const MachineDominatorTree &mdt) :
      MLI(mli), MDT(mdt) {}

    /// VisitLoop - Clear out any previous state and analyze the given loop.
    ///
    void VisitLoop(const MachineLoop *Loop) {
      assert(Deps.empty() && "stale loop dependencies");

      MachineBasicBlock *Header = Loop->getHeader();
      SmallSet<unsigned, 8> LoopLiveIns;
      for (MachineBasicBlock::livein_iterator LI = Header->livein_begin(),
           LE = Header->livein_end(); LI != LE; ++LI)
        LoopLiveIns.insert(*LI);

      const MachineDomTreeNode *Node = MDT.getNode(Header);
      const MachineBasicBlock *MBB = Node->getBlock();
      assert(Loop->contains(MBB) &&
             "Loop does not contain header!");
      VisitRegion(Node, MBB, Loop, LoopLiveIns);
    }

  private:
    void VisitRegion(const MachineDomTreeNode *Node,
                     const MachineBasicBlock *MBB,
                     const MachineLoop *Loop,
                     const SmallSet<unsigned, 8> &LoopLiveIns) {
      unsigned Count = 0;
      for (MachineBasicBlock::const_iterator I = MBB->begin(), E = MBB->end();
           I != E; ++I) {
        const MachineInstr *MI = I;
        if (MI->isDebugValue())
          continue;
        for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
          const MachineOperand &MO = MI->getOperand(i);
          if (!MO.isReg() || !MO.isUse())
            continue;
          unsigned MOReg = MO.getReg();
          if (LoopLiveIns.count(MOReg))
            Deps.insert(std::make_pair(MOReg, std::make_pair(&MO, Count)));
        }
        ++Count; // Not every iteration due to dbg_value above.
      }

      const std::vector<MachineDomTreeNode*> &Children = Node->getChildren();
      for (std::vector<MachineDomTreeNode*>::const_iterator I =
           Children.begin(), E = Children.end(); I != E; ++I) {
        const MachineDomTreeNode *ChildNode = *I;
        MachineBasicBlock *ChildBlock = ChildNode->getBlock();
        if (Loop->contains(ChildBlock))
          VisitRegion(ChildNode, ChildBlock, Loop, LoopLiveIns);
      }
    }
  };

  /// ScheduleDAGInstrs - A ScheduleDAG subclass for scheduling lists of
  /// MachineInstrs.
  class LLVM_LIBRARY_VISIBILITY ScheduleDAGInstrs : public ScheduleDAG {
    const MachineLoopInfo &MLI;
    const MachineDominatorTree &MDT;
    const MachineFrameInfo *MFI;
    const InstrItineraryData *InstrItins;

    /// isPostRA flag indicates vregs cannot be present.
    bool IsPostRA;

    /// Live Intervals provides reaching defs in preRA scheduling.
    LiveIntervals *LIS;

    /// After calling BuildSchedGraph, each machine instruction in the current
    /// scheduling region is mapped to an SUnit.
    DenseMap<MachineInstr*, SUnit*> MISUnitMap;

    /// UnitLatencies (misnamed) flag avoids computing def-use latencies, using
    /// the def-side latency only.
    bool UnitLatencies;

    /// Combine a SparseSet with a 1x1 vector to track physical registers.
    /// The SparseSet allows iterating over the (few) live registers for quickly
    /// comparing against a regmask or clearing the set.
    ///
    /// Storage for the map is allocated once for the pass. The map can be
    /// cleared between scheduling regions without freeing unused entries.
    class Reg2SUnitsMap {
      SparseSet<unsigned> PhysRegSet;
      std::vector<std::vector<SUnit*> > SUnits;
    public:
      typedef SparseSet<unsigned>::const_iterator const_iterator;

      // Allow iteration over register numbers (keys) in the map. If needed, we
      // can provide an iterator over SUnits (values) as well.
      const_iterator reg_begin() const { return PhysRegSet.begin(); }
      const_iterator reg_end() const { return PhysRegSet.end(); }

      /// Initialize the map with the number of registers.
      /// If the map is already large enough, no allocation occurs.
      /// For simplicity we expect the map to be empty().
      void setRegLimit(unsigned Limit);

      /// Returns true if the map is empty.
      bool empty() const { return PhysRegSet.empty(); }

      /// Clear the map without deallocating storage.
      void clear();

      bool contains(unsigned Reg) const { return PhysRegSet.count(Reg); }

      /// If this register is mapped, return its existing SUnits vector.
      /// Otherwise map the register and return an empty SUnits vector.
      std::vector<SUnit *> &operator[](unsigned Reg) {
        bool New = PhysRegSet.insert(Reg).second;
        assert((!New || SUnits[Reg].empty()) && "stale SUnits vector");
        (void)New;
        return SUnits[Reg];
      }

      /// Erase an existing element without freeing memory.
      void erase(unsigned Reg) {
        PhysRegSet.erase(Reg);
        SUnits[Reg].clear();
      }
    };
    /// Defs, Uses - Remember where defs and uses of each register are as we
    /// iterate upward through the instructions. This is allocated here instead
    /// of inside BuildSchedGraph to avoid the need for it to be initialized and
    /// destructed for each block.
    Reg2SUnitsMap Defs;
    Reg2SUnitsMap Uses;

    /// An individual mapping from virtual register number to SUnit.
    struct VReg2SUnit {
      unsigned VirtReg;
      SUnit *SU;

      VReg2SUnit(unsigned reg, SUnit *su): VirtReg(reg), SU(su) {}

      unsigned getSparseSetKey() const {
        return TargetRegisterInfo::virtReg2Index(VirtReg);
      }
    };
    /// Use SparseSet as a SparseMap by relying on the fact that it never
    /// compares ValueT's, only unsigned keys. This allows the set to be cleared
    /// between scheduling regions in constant time as long as ValueT does not
    /// require a destructor.
    typedef SparseSet<VReg2SUnit> VReg2SUnitMap;
    /// Track the last instructon in this region defining each virtual register.
    VReg2SUnitMap VRegDefs;

    /// PendingLoads - Remember where unknown loads are after the most recent
    /// unknown store, as we iterate. As with Defs and Uses, this is here
    /// to minimize construction/destruction.
    std::vector<SUnit *> PendingLoads;

    /// LoopRegs - Track which registers are used for loop-carried dependencies.
    ///
    LoopDependencies LoopRegs;

  protected:

    /// DbgValues - Remember instruction that preceeds DBG_VALUE.
    typedef std::vector<std::pair<MachineInstr *, MachineInstr *> >
      DbgValueVector;
    DbgValueVector DbgValues;
    MachineInstr *FirstDbgValue;

  public:
    MachineBasicBlock::iterator Begin;    // The beginning of the range to
                                          // be scheduled. The range extends
                                          // to InsertPos.
    unsigned InsertPosIndex;              // The index in BB of InsertPos.

    explicit ScheduleDAGInstrs(MachineFunction &mf,
                               const MachineLoopInfo &mli,
                               const MachineDominatorTree &mdt,
                               bool IsPostRAFlag,
                               LiveIntervals *LIS = 0);

    virtual ~ScheduleDAGInstrs() {}

    /// NewSUnit - Creates a new SUnit and return a ptr to it.
    ///
    SUnit *NewSUnit(MachineInstr *MI) {
#ifndef NDEBUG
      const SUnit *Addr = SUnits.empty() ? 0 : &SUnits[0];
#endif
      SUnits.push_back(SUnit(MI, (unsigned)SUnits.size()));
      assert((Addr == 0 || Addr == &SUnits[0]) &&
             "SUnits std::vector reallocated on the fly!");
      SUnits.back().OrigNode = &SUnits.back();
      return &SUnits.back();
    }


    /// Run - perform scheduling.
    ///
    void Run(MachineBasicBlock *bb,
             MachineBasicBlock::iterator begin,
             MachineBasicBlock::iterator end,
             unsigned endindex);

    /// BuildSchedGraph - Build SUnits from the MachineBasicBlock that we are
    /// input.
    void BuildSchedGraph(AliasAnalysis *AA);

    /// AddSchedBarrierDeps - Add dependencies from instructions in the current
    /// list of instructions being scheduled to scheduling barrier. We want to
    /// make sure instructions which define registers that are either used by
    /// the terminator or are live-out are properly scheduled. This is
    /// especially important when the definition latency of the return value(s)
    /// are too high to be hidden by the branch or when the liveout registers
    /// used by instructions in the fallthrough block.
    void AddSchedBarrierDeps();

    /// ComputeLatency - Compute node latency.
    ///
    virtual void ComputeLatency(SUnit *SU);

    /// ComputeOperandLatency - Override dependence edge latency using
    /// operand use/def information
    ///
    virtual void ComputeOperandLatency(SUnit *Def, SUnit *Use,
                                       SDep& dep) const;

    virtual MachineBasicBlock *EmitSchedule();

    /// StartBlock - Prepare to perform scheduling in the given block.
    ///
    virtual void StartBlock(MachineBasicBlock *BB);

    /// Schedule - Order nodes according to selected style, filling
    /// in the Sequence member.
    ///
    virtual void Schedule() = 0;

    /// FinishBlock - Clean up after scheduling in the given block.
    ///
    virtual void FinishBlock();

    virtual void dumpNode(const SUnit *SU) const;

    virtual std::string getGraphNodeLabel(const SUnit *SU) const;

    virtual std::string getDAGName() const;

  protected:
    SUnit *getSUnit(MachineInstr *MI) const {
      DenseMap<MachineInstr*, SUnit*>::const_iterator I = MISUnitMap.find(MI);
      if (I == MISUnitMap.end())
        return 0;
      return I->second;
    }

    void initSUnits();
    void addPhysRegDataDeps(SUnit *SU, const MachineOperand &MO);
    void addPhysRegDeps(SUnit *SU, unsigned OperIdx);
    void addVRegDefDeps(SUnit *SU, unsigned OperIdx);
    void addVRegUseDeps(SUnit *SU, unsigned OperIdx);

    VReg2SUnitMap::iterator findVRegDef(unsigned VirtReg) {
      return VRegDefs.find(TargetRegisterInfo::virtReg2Index(VirtReg));
    }
  };
}

#endif
