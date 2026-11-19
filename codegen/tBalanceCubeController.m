classdef tBalanceCubeController < matlabtest.coder.TestCase
%TBALANCECUBECONTROLLER MATLAB Test equivalence suite for MEX and SIL builds.

    properties
        MexBuildResults
        SilBuildResults
    end

    properties (TestParameter)
        tiltCase = struct( ...
            'upright',   struct('deg', single(0.0),  'enc', int32(0)), ...
            'pos5deg',   struct('deg', single(5.0),  'enc', int32(4)), ...
            'neg5deg',   struct('deg', single(-5.0), 'enc', int32(-4)), ...
            'fault35',   struct('deg', single(35.0), 'enc', int32(0)))
    end

    methods (TestClassSetup)
        function buildTargets(testCase)
            cfgSample = defaultControllerConfig();
            inputTypes = { ...
                coder.typeof(single(0)), ...
                coder.typeof(single(0)), ...
                coder.typeof(single(0)), ...
                coder.typeof(int32(0)), ...
                coder.typeof(false), ...
                coder.typeof(cfgSample)};

            testCase.MexBuildResults = build(testCase, 'balanceCubeControllerStep', ...
                Inputs=inputTypes, Configuration='mex');
            testCase.SilBuildResults = build(testCase, 'balanceCubeControllerStep', ...
                Inputs=inputTypes, Configuration='lib');
        end
    end

    methods (Test)
        function testSingleStepMexEquivalence(testCase, tiltCase)
            cfg = defaultControllerConfig();
            cfg.skipCalib = true;
            [axP, ayP] = synthImu45(tiltCase.deg);

            execRes = execute(testCase, testCase.MexBuildResults, ...
                Inputs={axP, ayP, single(0.0), tiltCase.enc, true, cfg});
            verifyExecutionMatchesMATLAB(testCase, execRes, ...
                AbsTol=single(1e-5), RelTol=single(1e-5));
        end

        function testSingleStepSilEquivalence(testCase, tiltCase)
            cfg = defaultControllerConfig();
            cfg.skipCalib = true;
            [axP, ayP] = synthImu45(tiltCase.deg);

            execRes = execute(testCase, testCase.SilBuildResults, ...
                Inputs={axP, ayP, single(0.0), tiltCase.enc, true, cfg});
            verifyExecutionMatchesMATLAB(testCase, execRes, ...
                AbsTol=single(1e-5), RelTol=single(1e-5));
        end
    end
end

function [axP, ayP] = synthImu45(degVal)
    gAcc = single(9.80665);
    c45 = single(0.70710678);
    th = degVal * single(pi / 180.0);
    axD = gAcc * sin(th);
    ayD = gAcc * cos(th);
    axP = c45 * (axD + ayD);
    ayP = c45 * (-axD + ayD);
end
