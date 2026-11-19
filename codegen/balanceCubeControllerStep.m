function [pwmDuty, in1High, in2High, modeOut, thetaHatDeg, omegaWRads] = balanceCubeControllerStep( ...
    axMps2, ayMps2, gzRads, encTicksDelta, resetState, cfgParams) %#codegen
%BALANCECUBECONTROLLERSTEP 200 Hz single-precision controller for ESP32 1D Balance Cube.
    arguments
        axMps2        (1,1) single
        ayMps2        (1,1) single
        gzRads        (1,1) single
        encTicksDelta (1,1) int32
        resetState    (1,1) logical
        cfgParams     (1,1) struct
    end

    coder.allowpcode('plain');
    coder.cstructname(cfgParams, 'BalanceCubeConfig_T');

    persistent isInit calibCount gzSum gzBias thetaHat omegaWFilt intWheelW ...
               uprightCount modeState

    % 1. Accelerometer tilt projection (45 deg rotation if IMU aligns with square edge)
    if cfgParams.imuRotated45deg
        c45 = single(0.70710678);
        axDiag = c45 * (axMps2 - ayMps2);
        ayDiag = c45 * (axMps2 + ayMps2);
    else
        axDiag = axMps2;
        ayDiag = ayMps2;
    end
    thetaAcc = atan2(axDiag, ayDiag) - cfgParams.mountTrimRad;

    % 2. Unconditional initialization for MATLAB Coder persistent analysis, plus runtime reset
    didReset = resetState;
    if isempty(isInit)
        isInit = true;
        didReset = true;
        calibCount = uint16(0);
        gzSum = single(0.0);
        gzBias = single(0.0);
        thetaHat = thetaAcc;
        omegaWFilt = single(0.0);
        intWheelW = single(0.0);
        uprightCount = uint16(0);
        if cfgParams.skipCalib
            modeState = uint8(2);
        else
            modeState = uint8(0);
        end
    elseif resetState
        calibCount = uint16(0);
        gzSum = single(0.0);
        gzBias = single(0.0);
        thetaHat = thetaAcc;
        omegaWFilt = single(0.0);
        intWheelW = single(0.0);
        uprightCount = uint16(0);
        if cfgParams.skipCalib
            modeState = uint8(2);
        else
            modeState = uint8(0);
        end
    end

    Ts = cfgParams.Ts;

    % 3. Quadrature encoder angular velocity + 1st-order LPF
    omegaWRaw = (single(6.2831853) * single(encTicksDelta)) / (cfgParams.cpr4x * Ts);
    beta = cfgParams.encLpfBeta;
    omegaWFilt = (single(1.0) - beta) * omegaWFilt + beta * omegaWRaw;

    % 4. Complementary tilt angle filter
    thetaDot = gzRads - gzBias;
    if ~didReset
        alpha = cfgParams.cfAlpha;
        thetaHat = alpha * (thetaHat + thetaDot * Ts) + (single(1.0) - alpha) * thetaAcc;
    end

    % 5. Four-mode supervisory state machine (0=CALIB, 1=IDLE, 2=BALANCE, 3=FAULT)
    Vcmd = single(0.0);
    switch modeState
        case uint8(0)
            thetaHat = thetaAcc;
            gzSum = gzSum + gzRads;
            calibCount = calibCount + uint16(1);
            if calibCount >= uint16(400)
                gzBias = gzSum / single(400.0);
                modeState = uint8(1);
                uprightCount = uint16(0);
            end

        case uint8(1)
            intWheelW = single(0.0);
            if (abs(thetaHat) < cfgParams.engageAngleRad) && (abs(thetaDot) < single(0.35))
                uprightCount = uprightCount + uint16(1);
                if uprightCount >= uint16(20)
                    modeState = uint8(2);
                end
            else
                uprightCount = uint16(0);
            end

        case uint8(2)
            if abs(thetaHat) > cfgParams.catchLimitRad
                modeState = uint8(3);
                intWheelW = single(0.0);
                Vcmd = single(0.0);
            else
                Vlqr = -(cfgParams.K(1) * thetaHat + ...
                         cfgParams.K(2) * thetaDot + ...
                         cfgParams.K(3) * omegaWFilt + ...
                         cfgParams.K(4) * intWheelW);

                % Conditional anti-windup on flywheel speed integral
                if abs(Vlqr) < cfgParams.VmaxEff
                    intWheelW = intWheelW + omegaWFilt * Ts;
                    intWheelW = min(cfgParams.intWMax, max(-cfgParams.intWMax, intWheelW));
                end
                Vcmd = min(cfgParams.VmaxEff, max(-cfgParams.VmaxEff, Vlqr));
            end

        otherwise
            intWheelW = single(0.0);
            Vcmd = single(0.0);
            if (abs(thetaHat) < cfgParams.engageAngleRad) && (abs(thetaDot) < single(0.35))
                modeState = uint8(1);
                uprightCount = uint16(0);
            end
    end

    % 6. H-bridge PWM & direction pin mapping (smooth anti-chatter stiction ramp)
    absV = abs(Vcmd);
    if absV < single(0.02)
        pwmDuty = single(0.0);
        in1High = false;
        in2High = false;
    else
        stictionScale = min(single(1.0), absV / single(0.25));
        veffCmd = absV + stictionScale * cfgParams.Vdeadband;
        pwmDuty = min(single(1.0), veffCmd / cfgParams.VmaxEff);
        in1High = (Vcmd > single(0.0));
        in2High = (Vcmd < single(0.0));
    end

    modeOut = modeState;
    thetaHatDeg = thetaHat * single(57.29578);
    omegaWRads = omegaWFilt;
end
