/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2015 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#include "exception.hh"
#include "Text.hh"
#include "floats.hh"
#include "global.hh"
#include "interpreter_code_container.hh"
#include "interpreter_optimizer.hh"
#include "interpreter_instructions.hh"

using namespace std;

/*
Interpretor : 
 
 - multiple unneeded cast are eliminated in CastNumInst
 - 'faustpower' recoded as pow(x,y) in powprim.hh

*/

template <class T> map <string, FIRInstruction::Opcode> InterpreterInstVisitor<T>::gMathLibTable;

InterpreterCodeContainer::InterpreterCodeContainer(const string& name, int numInputs, int numOutputs)
{
    initializeCodeContainer(numInputs, numOutputs);
    fKlassName = name;
    
    // Allocate one static visitor
    if (!gGlobal->gInterpreterVisitor) {
        gGlobal->gInterpreterVisitor = new InterpreterInstVisitor<float>();
    }
    
    // Initializations for FIRInstructionMathOptimizer pass
    
    //===============
    // Math
    //===============
    
    // Init heap opcode
    for (int i = FIRInstruction::kAddReal; i <= FIRInstruction::kXORInt; i++) {
        FIRInstruction::gFIRMath2Heap[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init stack opcode
    for (int i = FIRInstruction::kAddReal; i <= FIRInstruction::kXORInt; i++) {
        FIRInstruction::gFIRMath2Stack[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAddRealStack - FIRInstruction::kAddReal));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealStack - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init stack/value opcode
    for (int i = FIRInstruction::kAddReal; i <= FIRInstruction::kXORInt; i++) {
        FIRInstruction::gFIRMath2StackValue[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAddRealStackValue - FIRInstruction::kAddReal));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealStackValue - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init Value opcode
    for (int i = FIRInstruction::kAddReal; i <= FIRInstruction::kXORInt; i++) {
        FIRInstruction::gFIRMath2Value[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAddRealValue - FIRInstruction::kAddReal));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealValue - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init Value opcode (non commutative operation)
    for (int i = FIRInstruction::kAddReal; i <= FIRInstruction::kXORInt; i++) {
        FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAddRealValue - FIRInstruction::kAddReal));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealValue - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Manually set inverted versions
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kSubReal] = FIRInstruction::kSubRealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kSubInt] = FIRInstruction::kSubIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kDivReal] = FIRInstruction::kDivRealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kDivInt] = FIRInstruction::kDivIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kRemReal] = FIRInstruction::kRemRealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kRemInt] = FIRInstruction::kRemIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kLshInt] = FIRInstruction::kLshIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kRshInt] = FIRInstruction::kRshIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kGTInt] = FIRInstruction::kGTIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kLTInt] = FIRInstruction::kLTIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kGEInt] = FIRInstruction::kGEIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kLEInt] = FIRInstruction::kLEIntValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kGTReal] = FIRInstruction::kGTRealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kLTReal] = FIRInstruction::kLTRealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kGEReal] = FIRInstruction::kGERealValueInvert;
    FIRInstruction::gFIRMath2ValueInvert[FIRInstruction::kLEReal] = FIRInstruction::kLERealValueInvert;
    
    //===============
    // EXTENDED math
    //===============
    
    // Init unary extended math heap opcode
    for (int i = FIRInstruction::kAbs; i <= FIRInstruction::kTanhf; i++) {
        FIRInstruction::gFIRExtendedMath2Heap[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAbsHeap - FIRInstruction::kAbs));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init binary extended math heap opcode
    for (int i = FIRInstruction::kAtan2f; i <= FIRInstruction::kMinf; i++) {
        FIRInstruction::gFIRExtendedMath2Heap[FIRInstruction::Opcode(i)]
        = FIRInstruction::Opcode(i + (FIRInstruction::kAtan2fHeap - FIRInstruction::kAtan2f));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    
    // Init binary extended math stack opcode
    for (int i = FIRInstruction::kAtan2f; i <= FIRInstruction::kMinf; i++) {
        FIRInstruction::gFIRExtendedMath2Stack[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAtan2fStack - FIRInstruction::kAtan2f));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init binary extended math stack/value opcode
    for (int i = FIRInstruction::kAtan2f; i <= FIRInstruction::kMinf; i++) {
        FIRInstruction::gFIRExtendedMath2StackValue[FIRInstruction::Opcode(i)]
        = FIRInstruction::Opcode(i + (FIRInstruction::kAtan2fStackValue - FIRInstruction::kAtan2f));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
 
    // Init unary math Value opcode
    for (int i = FIRInstruction::kAtan2f; i <= FIRInstruction::kMinf; i++) {
        FIRInstruction::gFIRExtendedMath2Value[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAtan2fValue - FIRInstruction::kAtan2f));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Init unary math Value opcode : non commutative operations
    for (int i = FIRInstruction::kAtan2f; i <= FIRInstruction::kPowf; i++) {
        FIRInstruction::gFIRExtendedMath2ValueInvert[FIRInstruction::Opcode(i)]
            = FIRInstruction::Opcode(i + (FIRInstruction::kAtan2fValueInvert - FIRInstruction::kAtan2f));
        //std::cout << gFIRInstructionTable[i + (FIRInstruction::kAddRealHeap - FIRInstruction::kAddReal)] << std::endl;
    }
    
    // Test
    
    /*
    std::cout << "gFIRExtendedMath2Heap" << std::endl;
    for (int i = FIRInstruction::kAbs; i <= FIRInstruction::kTanhf; i++) {
        if (FIRInstruction::gFIRExtendedMath2Heap.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2Heap.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
            << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2Heap[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    std::cout << "gFIRExtendedMath2Heap" << std::endl << std::endl;
    for (int i = FIRInstruction::kAbsHeap; i <= FIRInstruction::kMinf; i++) {
        if (FIRInstruction::gFIRExtendedMath2Heap.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2Heap.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
                        << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2Heap[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    
    std::cout << "gFIRExtendedMath2Stack" << std::endl << std::endl;
    for (int i = FIRInstruction::kAbsHeap; i <= FIRInstruction::kMinf; i++) {
        if (FIRInstruction::gFIRExtendedMath2Stack.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2Stack.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
            << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2Stack[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    
    std::cout << "gFIRExtendedMath2StackValue" << std::endl << std::endl;
    for (int i = FIRInstruction::kAbsHeap; i <= FIRInstruction::kMinf; i++) {
        if (FIRInstruction::gFIRExtendedMath2StackValue.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2StackValue.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
            << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2StackValue[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    
    std::cout << "gFIRExtendedMath2Value" << std::endl << std::endl;
    for (int i = FIRInstruction::kAbsHeap; i <= FIRInstruction::kMinf; i++) {
        if (FIRInstruction::gFIRExtendedMath2Value.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2Value.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
            << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2Value[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    
    std::cout << "gFIRExtendedMath2ValueInvert" << std::endl << std::endl;
    for (int i = FIRInstruction::kAbsHeap; i <= FIRInstruction::kMinf; i++) {
        if (FIRInstruction::gFIRExtendedMath2ValueInvert.find(FIRInstruction::Opcode(i)) !=  FIRInstruction::gFIRExtendedMath2ValueInvert.end()) {
            std::cout   << gFIRInstructionTable[FIRInstruction::Opcode(i)] << " ==> "
            << gFIRInstructionTable[FIRInstruction::gFIRExtendedMath2ValueInvert[FIRInstruction::Opcode(i)]] << std::endl;
        }
    }
    */
}

CodeContainer* InterpreterCodeContainer::createScalarContainer(const string& name, int sub_container_type)
{
    return new InterpreterScalarCodeContainer(name, 0, 1, sub_container_type);
}

CodeContainer* InterpreterCodeContainer::createContainer(const string& name, int numInputs, int numOutputs)
{
    CodeContainer* container;

    if (gGlobal->gOpenCLSwitch) {
        throw faustexception("ERROR : OpenCL not supported for Interpreter\n");
    }
    if (gGlobal->gCUDASwitch) {
        throw faustexception("ERROR : CUDA not supported for Interpreter\n");
    }

    if (gGlobal->gOpenMPSwitch) {
        throw faustexception("ERROR : OpenMP not supported for Interpreter\n");
    } else if (gGlobal->gSchedulerSwitch) {
        throw faustexception("ERROR : Scheduler not supported for Interpreter\n");
    } else if (gGlobal->gVectorSwitch) {
        throw faustexception("ERROR : Vector not supported for Interpreter\n");
    } else {
        container = new InterpreterScalarCodeContainer(name, numInputs, numOutputs, kInt);
    }

    return container;
}

// Scalar
InterpreterScalarCodeContainer::InterpreterScalarCodeContainer(const string& name,  int numInputs, int numOutputs, int sub_container_type)
    :InterpreterCodeContainer(name, numInputs, numOutputs)
{
     fSubContainerType = sub_container_type;
}

InterpreterScalarCodeContainer::~InterpreterScalarCodeContainer()
{}

void InterpreterCodeContainer::produceInternal()
{
    /*
    //cout << "generateGlobalDeclarations" << endl;
    generateGlobalDeclarations(gGlobal->gInterpreterVisitor);
    
    //cout << "generateDeclarations" << endl;
    generateDeclarations(gGlobal->gInterpreterVisitor);
    */
    
}

FIRBlockInstruction<float>* InterpreterCodeContainer::testOptimizer(FIRBlockInstruction<float>* block, int& size)
{
    
    cout << "fComputeDSPBlock size = " << block->size() << endl;
    
    // 1) optimize indexed 'heap' load/store in normal load/store
    FIRInstructionLoadStoreOptimizer<float> opt1;
    block = FIRInstructionOptimizer<float>::optimize(block, opt1);
    
    cout << "fComputeDSPBlock size = " << block->size() << endl;
    
    // 2) then pptimize simple 'heap' load/store in move
    FIRInstructionMoveOptimizer<float> opt2;
    block = FIRInstructionOptimizer<float>::optimize(block, opt2);
    
    cout << "fComputeDSPBlock size = " << block->size() << endl;
    
    // 3) optimize 'cast' in heap cast
    FIRInstructionCastOptimizer<float> opt3;
    block = FIRInstructionOptimizer<float>::optimize(block, opt3);
    
    cout << "fComputeDSPBlock size = " << block->size() << endl;
    
    // 4) them optimize 'heap' and 'Value' math operations
    FIRInstructionMathOptimizer<float> opt4;
    block = FIRInstructionOptimizer<float>::optimize(block, opt4);
    
    cout << "fComputeDSPBlock size = " << block->size() << endl << endl;
    
    size = block->size();
    return block;
}

interpreter_dsp_factory* InterpreterCodeContainer::produceFactoryFloat()
{
    //cout << "InterpreterCodeContainer::produceModuleFloat() " << fNumInputs << " " << fNumOutputs << endl;
    
    // Add "fSamplingFreq" variable at offset 0 in HEAP
    if (!fGeneratedSR) {
        fDeclarationInstructions->pushBackInst(InstBuilder::genDecStructVar("fSamplingFreq", InstBuilder::genBasicTyped(Typed::kInt)));
    }
    
    // "count" variable
    fDeclarationInstructions->pushBackInst(InstBuilder::genDecStructVar("count", InstBuilder::genBasicTyped(Typed::kInt)));
    
    //cout << "generateGlobalDeclarations" << endl;
    generateGlobalDeclarations(gGlobal->gInterpreterVisitor);

    //cout << "generateDeclarations" << endl;
    generateDeclarations(gGlobal->gInterpreterVisitor);
    
    //generateAllocate(gGlobal->gInterpreterVisitor);
    //generateDestroy(gGlobal->gInterpreterVisitor);
    
    //cout << "generateStaticInit" << endl;
    generateStaticInit(gGlobal->gInterpreterVisitor);
    
    //cout << "generateInit" << endl;
    generateInit(gGlobal->gInterpreterVisitor);
    
    FIRBlockInstruction<float>* init_block = gGlobal->gInterpreterVisitor->fCurrentBlock;
    gGlobal->gInterpreterVisitor->fCurrentBlock = new FIRBlockInstruction<float>();
    
    //cout << "generateUserInterface" << endl;
    generateUserInterface(gGlobal->gInterpreterVisitor);
    
    // Generates local variables declaration and setup
    //cout << "generateComputeBlock" << endl;
    generateComputeBlock(gGlobal->gInterpreterVisitor);
    
    FIRBlockInstruction<float>* compute_control_block = gGlobal->gInterpreterVisitor->fCurrentBlock;
    gGlobal->gInterpreterVisitor->fCurrentBlock = new FIRBlockInstruction<float>();

    // Generates one single scalar loop
    //cout << "generateScalarLoop" << endl;
    ForLoopInst* loop = fCurLoop->generateScalarLoop(fFullCount);
    
    loop->accept(gGlobal->gInterpreterVisitor);
    FIRBlockInstruction<float>* compute_dsp_block = gGlobal->gInterpreterVisitor->fCurrentBlock;
   
    //generateCompute(0);
    //generateComputeFunctions(gGlobal->gInterpreterVisitor);
    
    // Add kReturn in blocks
    init_block->push(new FIRBasicInstruction<float>(FIRInstruction::kReturn));
    compute_control_block->push(new FIRBasicInstruction<float>(FIRInstruction::kReturn));
    compute_dsp_block->push(new FIRBasicInstruction<float>(FIRInstruction::kReturn));
    
    // Bytecode optimization
    
    /*
    cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl;
    
    FIRInstructionCopyOptimizer<float> opt0;
    init_block = FIRInstructionOptimizer<float>::optimize(init_block, opt0);
    compute_control_block = FIRInstructionOptimizer<float>::optimize(compute_control_block, opt0);
    compute_dsp_block = FIRInstructionOptimizer<float>::optimize(compute_dsp_block, opt0);
    */
    
    //cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl;
    
    
    // 1) optimize indexed 'heap' load/store in normal load/store
    FIRInstructionLoadStoreOptimizer<float> opt1;
    init_block = FIRInstructionOptimizer<float>::optimize(init_block, opt1);
    compute_control_block = FIRInstructionOptimizer<float>::optimize(compute_control_block, opt1);
    compute_dsp_block = FIRInstructionOptimizer<float>::optimize(compute_dsp_block, opt1);
    
    //cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl;
    
    // 2) then pptimize simple 'heap' load/store in move
    FIRInstructionMoveOptimizer<float> opt2;
    init_block = FIRInstructionOptimizer<float>::optimize(init_block, opt2);
    compute_control_block = FIRInstructionOptimizer<float>::optimize(compute_control_block, opt2);
    compute_dsp_block = FIRInstructionOptimizer<float>::optimize(compute_dsp_block, opt2);
    
    //cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl;
    
    // 3) optimize 'cast' in heap cast
    FIRInstructionCastOptimizer<float> opt3;
    init_block = FIRInstructionOptimizer<float>::optimize(init_block, opt3);
    compute_control_block = FIRInstructionOptimizer<float>::optimize(compute_control_block, opt3);
    compute_dsp_block = FIRInstructionOptimizer<float>::optimize(compute_dsp_block, opt3);
 
    cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl;
    
    // 4) them optimize 'heap' and 'Value' math operations
    FIRInstructionMathOptimizer<float> opt4;
    init_block = FIRInstructionOptimizer<float>::optimize(init_block, opt4);
    compute_control_block = FIRInstructionOptimizer<float>::optimize(compute_control_block, opt4);
    compute_dsp_block = FIRInstructionOptimizer<float>::optimize(compute_dsp_block, opt4);
    
    cout << "fComputeDSPBlock size = " << compute_dsp_block->size() << endl << endl;
    
     
    // TODO
    /*
    int int_index = 0;
    int real_index = 0;
    compute_dsp_block->stackMove(int_index, real_index);
    printf("fComputeDSPBlock int stack = %d real stack = %d\n", int_index, real_index);
    */
   
    /*
    // Test reader/writer
    interpreter_dsp_factory* factory = new interpreter_dsp_factory(fKlassName,
                                                                   INTERP_VERSION,
                                                                   fNumInputs, fNumOutputs,
                                                                   gGlobal->gInterpreterVisitor->fIntHeapOffset,
                                                                   gGlobal->gInterpreterVisitor->fRealHeapOffset,
                                                                   gGlobal->gInterpreterVisitor->fSROffset,
                                                                   gGlobal->gInterpreterVisitor->fUserInterfaceBlock,
                                                                   init_block,
                                                                   compute_control_block,
                                                                   compute_dsp_block);
    
    
    cout << "writeDSPInterpreterFactoryToMachine" << endl;
    string machine_code = writeDSPInterpreterFactoryToMachine(factory);
    
    cout << "readDSPInterpreterFactoryFromMachine" << endl;
    interpreter_dsp_factory* factory1 = readDSPInterpreterFactoryFromMachine(machine_code);
    
    return factory1;
    */
    
    return new interpreter_dsp_factory(fKlassName,
                                        INTERP_FILE_VERSION,
                                        fNumInputs, fNumOutputs,
                                        gGlobal->gInterpreterVisitor->fIntHeapOffset,
                                        gGlobal->gInterpreterVisitor->fRealHeapOffset,
                                        gGlobal->gInterpreterVisitor->fSROffset,
                                        gGlobal->gInterpreterVisitor->fCountOffset,
                                        gGlobal->gInterpreterVisitor->fUserInterfaceBlock,
                                        init_block,
                                        compute_control_block,
                                        compute_dsp_block);
}

/*
interpreter_dsp_aux<double>* InterpreterCodeContainer::produceModuleDouble()
{
    cout << "InterpreterCodeContainer::produceModuleDouble()" << endl;
    //return new interpreter_dsp_aux<double>(fNumInputs, fNumOutputs, 0, 0, NULL, NULL, NULL, NULL);
    return NULL;
}

interpreter_dsp_aux<quad>* InterpreterCodeContainer::produceModuleQuad()
{
    cout << "InterpreterCodeContainer::produceModuleQuad()" << endl;
    //return new interpreter_dsp_aux<quad>(fNumInputs, fNumOutputs, 0, 0, NULL, NULL, NULL, NULL);
    return NULL;
}
*/

void InterpreterCodeContainer::produceClass()
{
    /*
    printf("InterpreterCodeContainer::produceClass\n");
    
    // Add "fSamplingFreq" variable at offset 0 in HEAP
    pushDeclare(InstBuilder::genDecStructVar("fSamplingFreq", InstBuilder::genBasicTyped(Typed::kInt)));
    
    generateGlobalDeclarations(gGlobal->gInterpreterVisitor);

    generateDeclarations(gGlobal->gInterpreterVisitor);
    
    //generateAllocate(gGlobal->gInterpreterVisitor);
    //generateDestroy(gGlobal->gInterpreterVisitor);
    
    generateStaticInit(gGlobal->gInterpreterVisitor);
    
    //generateInit(gGlobal->gInterpreterVisitor);
    
    generateUserInterface(gGlobal->gInterpreterVisitor);
    
    generateCompute(0);
    
    //generateComputeFunctions(gGlobal->gInterpreterVisitor);
    */
}

void InterpreterCodeContainer::produceInfoFunctions(int tabs, const string& classname, bool isvirtual)
{}

void InterpreterScalarCodeContainer::generateCompute(int n)
{
    // Generates local variables declaration and setup
    //generateComputeBlock(gGlobal->gInterpreterVisitor);

    // Generates one single scalar loop
    ForLoopInst* loop = fCurLoop->generateScalarLoop(fFullCount);
    loop->accept(gGlobal->gInterpreterVisitor);
}
