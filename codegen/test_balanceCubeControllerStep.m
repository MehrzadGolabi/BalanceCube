%% test_balanceCubeControllerStep.m
% Deterministic stateful verification script for balanceCubeControllerStep.
% Used directly in MATLAB and via coder.runTest for MEX and SIL equivalence.

cfg = defaultControllerConfig();
gAcc = single(9.80665);
c45 = single(0.70710678);
biasRadS = single(0.035);

% Phase 1: 400 ticks calibrating while resting at +20 deg
thRad = single(deg2rad(20.0));
for k = 1:400
    axD = gAcc * sin(thRad);
    ayD = gAcc * cos(thRad);
    axP = c45 * (axD + ayD);
    ayP = c45 * (-axD + ayD);
    [duty, in1, in2, mode, thDeg, ~] = balanceCubeControllerStep( ...
        axP, ayP, biasRadS, int32(0), (k == 1), cfg);
    assert(duty == single(0.0), 'PWM must be zero during calibration');
    assert(~in1 && ~in2, 'H-bridge must be off during calibration');
    assert(isfinite(thDeg), 'Estimated angle must be finite');
    if k < 400
        assert(mode == uint8(0), 'Expected MODE_CALIB before tick 400');
    else
        assert(mode == uint8(1), 'Expected MODE_IDLE at tick 400');
    end
end

% Phase 2: Lift upright (+1 deg) on tick 1 and hold steady for 20 ticks -> enter BALANCE
thRad = single(deg2rad(1.0));
for k = 1:21
    axD = gAcc * sin(thRad);
    ayD = gAcc * cos(thRad);
    axP = c45 * (axD + ayD);
    ayP = c45 * (-axD + ayD);
    % On first tick of lift, step gyro synchronously so CF moves to +1 deg
    if k == 1
        gz = biasRadS + single(deg2rad(1.0 - 20.0)) / cfg.Ts;
    else
        gz = biasRadS;
    end
    [duty, in1, in2, mode, ~, ~] = balanceCubeControllerStep( ...
        axP, ayP, gz, int32(0), false, cfg);
    if k < 21
        assert(mode == uint8(1), 'Expected MODE_IDLE while stabilizing');
        assert(duty == single(0.0), 'Expected zero PWM in MODE_IDLE');
    else
        assert(mode == uint8(2), 'Expected transition to MODE_BALANCE');
        assert(in1 || in2 || duty == single(0.0), 'Consistent H-bridge state');
    end
end

% Phase 3: Tilted to +4 deg with wheel spinning at +10 ticks/step -> positive drive
thTarget = single(deg2rad(4.0));
axD = gAcc * sin(thTarget);
ayD = gAcc * cos(thTarget);
axP = c45 * (axD + ayD);
ayP = c45 * (-axD + ayD);
gzStep = biasRadS + (thTarget - single(deg2rad(1.0))) / cfg.Ts;
[duty, in1, in2, mode, thDeg, wRads] = balanceCubeControllerStep( ...
    axP, ayP, gzStep, int32(10), false, cfg);
assert(mode == uint8(2), 'Should remain in MODE_BALANCE at +4 deg');
assert(abs(thDeg - single(4.0)) < single(0.5), 'Complementary filter tracking check');
assert(wRads > single(0.0), 'Positive encoder ticks must yield positive wheel speed');
assert(in1 && ~in2, 'Positive tilt requires +V_cmd (IN1=1, IN2=0)');
assert(duty > single(0.5) && duty <= single(1.0), 'Expected strong stabilizing PWM duty');

% Phase 4: Knocked past 35 deg -> immediate MODE_FAULT cutoff
thFault = single(deg2rad(35.0));
axD = gAcc * sin(thFault);
ayD = gAcc * cos(thFault);
axP = c45 * (axD + ayD);
ayP = c45 * (-axD + ayD);
gzFault = biasRadS + (thFault - thTarget) / cfg.Ts;
[duty, in1, in2, mode, ~, ~] = balanceCubeControllerStep( ...
    axP, ayP, gzFault, int32(0), false, cfg);
assert(mode == uint8(3), 'Must trip MODE_FAULT above 30 deg');
assert(duty == single(0.0) && ~in1 && ~in2, 'Must disable motor in MODE_FAULT');

% Phase 5: Returned upright -> 1st tick brings theta=0, 2nd tick holds steady -> MODE_IDLE
axD = single(0.0);
ayD = gAcc;
axP = c45 * (axD + ayD);
ayP = c45 * (-axD + ayD);
gzBack = biasRadS - thFault / cfg.Ts;
[~, ~, ~, ~, ~, ~] = balanceCubeControllerStep( ...
    axP, ayP, gzBack, int32(0), false, cfg);
[~, ~, ~, mode, ~, ~] = balanceCubeControllerStep( ...
    axP, ayP, biasRadS, int32(0), false, cfg);
assert(mode == uint8(1), 'Must recover to MODE_IDLE once upright and steady');

% Phase 6: Direct skipCalib mode used by Simscape SIL loop
cfgSim = cfg;
cfgSim.skipCalib = true;
thTest = single(deg2rad(5.0));
axD = gAcc * sin(thTest);
ayD = gAcc * cos(thTest);
axP = c45 * (axD + ayD);
ayP = c45 * (-axD + ayD);
[duty, in1, in2, mode, thDeg, ~] = balanceCubeControllerStep( ...
    axP, ayP, single(0.0), int32(0), true, cfgSim);
assert(mode == uint8(2), 'skipCalib=true must start directly in MODE_BALANCE');
assert(abs(thDeg - single(5.0)) < single(0.05), 'Initial tilt from accel must match 5 deg');
assert(in1 && ~in2 && duty > single(0.9), '5 deg tilt must saturate near full +V_cmd');
