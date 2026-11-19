function vCmd = simulinkMcuControllerHook(u)
%SIMULINKMCUCONTROLLERHOOK Simulink-to-SIL C99 bridge with synthetic IMU & encoder quantization.
%   u = [theta_rad; theta_dot_rads; wheel_w_rads; sim_time_s]

    persistent cfgSim phiAccum prevTicks

    th = u(1);
    thd = u(2);
    wWheel = u(3);
    tNow = u(4);

    if isempty(cfgSim) || (tNow < 0.0025)
        cfgSim = defaultControllerConfig();
        cfgSim.skipCalib = true;
        phiAccum = 0.0;
        prevTicks = int32(0);
        isStart = true;
    else
        isStart = false;
    end

    Ts = double(cfgSim.Ts);

    % Quantize flywheel angle to 4x quadrature encoder ticks (275 ticks/rev)
    if ~isStart
        phiAccum = phiAccum + wWheel * Ts;
    end
    ticksNow = int32(round(phiAccum * (double(cfgSim.cpr4x) / (2.0 * pi))));
    encTicksDelta = ticksNow - prevTicks;
    prevTicks = ticksNow;

    % Synthesize 45-deg PCB-rotated MPU6050 readings with realistic sensor noise
    gAcc = 9.80665;
    axDiag = gAcc * sin(th);
    ayDiag = gAcc * cos(th);
    c45 = 0.70710678;

    noiseAx = 0.05 * sin(137.0 * tNow);
    noiseAy = 0.05 * cos(211.0 * tNow);
    noiseGz = 0.003 * sin(173.0 * tNow);

    axPcb = single(c45 * (axDiag + ayDiag) + noiseAx);
    ayPcb = single(c45 * (-axDiag + ayDiag) + noiseAy);
    gzMeas = single(thd + noiseGz);

    % Execute compiled Embedded Coder C99 SIL binary
    [pwmDuty, in1High, in2High, ~, ~, ~] = balanceCubeControllerStep_sil( ...
        axPcb, ayPcb, gzMeas, encTicksDelta, isStart, cfgSim);

    vCmd = (double(in1High) - double(in2High)) * double(pwmDuty) * double(cfgSim.VmaxEff);
end
