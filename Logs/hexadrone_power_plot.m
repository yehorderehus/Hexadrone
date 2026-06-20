% --- 0. Housekeeping ---
close all;  % Closes all open figure windows
clear;      % Clears all variables from the workspace
clc;        % Clears the command window

% --- 1. Load the data ---
data = readtable('power_11_05.csv', 'VariableNamingRule', 'preserve');

% --- 2. Process the Timestamp ---
cleanTimeStr = erase(data.Timestamp, ["[", "]"]);
timeAxis = duration(cleanTimeStr); 
totalDuration = timeAxis(end) - timeAxis(1);

% --- 3. Configuration & Calculations ---
numServos = 18;
servoVoltage = 6.8;
efficiency = 0.90;      % 90% efficiency for the buck converters
maxStallCurrent = 1.2;  % Defined stall current limit
maxBatteryVolt = 24.6;  % Fully charged 6S LiPo limit

% Convert Battery Power to individual servo current
totalPowerOut = data.("Power(W)") * efficiency; 
totalCurrentAt68V = totalPowerOut / servoVoltage;
currentPerServo = totalCurrentAt68V / numServos;

% --- 4. Create the Combined Figure ---
figure('Name', 'Hexadrone Power and Servo Analysis', 'Units', 'normalized', 'Position', [0.1, 0.1, 0.8, 0.8]);

% --- Subplot 1: Voltage ---
ax1 = subplot(4, 1, 1);
vData = data.("Voltage(V)");
vStart = vData(1);
vEnd = vData(end);
vDiff = vStart - vEnd;

plot(timeAxis, vData, 'LineWidth', 1.5, 'Color', [0 0.447 0.741]);
hold on;

% Highlight Start and End points
plot(timeAxis(1), vStart, 'go', 'MarkerFaceColor', 'g', 'MarkerSize', 8);
text(timeAxis(1), vStart, ['  Start: ', num2str(vStart, '%.2f'), 'V'], 'VerticalAlignment', 'bottom');
plot(timeAxis(end), vEnd, 'ro', 'MarkerFaceColor', 'r', 'MarkerSize', 8);
text(timeAxis(end), vEnd, ['  End: ', num2str(vEnd, '%.2f'), 'V'], 'VerticalAlignment', 'top', 'HorizontalAlignment', 'right');

% References and Labels
yline(maxBatteryVolt, '--r', 'Max Voltage (24.6V)', 'LabelVerticalAlignment', 'bottom');
ylabel('Voltage (V)');
title(['System Voltage (Battery) - Total Drop: ', num2str(vDiff, '%.2f'), ' V']);
grid on;
ylim([min(vData)-1, maxBatteryVolt+1]);

% --- Subplot 2: Power ---
ax2 = subplot(4, 1, 2);
plot(timeAxis, data.("Power(W)"), 'LineWidth', 1.5, 'Color', [0.85 0.325 0.098]);
ylabel('Power (W)');
title('Total Power Consumption');
grid on;

% --- Subplot 3: Servo Current ---
ax3 = subplot(4, 1, 3);
plot(timeAxis, currentPerServo, 'LineWidth', 1.5, 'Color', [0.635 0.078 0.184]);
hold on;
yline(mean(currentPerServo), '--k', ['Avg: ', num2str(mean(currentPerServo), '%.2f'), 'A']);
yline(maxStallCurrent, '-r', 'STALL LIMIT (1.2A)', 'LineWidth', 1.5);
ylabel('Current / Servo (A)');
title('Estimated Current per Servo (@ 6.8V)');
grid on;
ylim([0, maxStallCurrent + 0.3]);

% --- Subplot 4: Capacity ---
ax4 = subplot(4, 1, 4);
cData = data.("Capacity(mAh)");
cStart = cData(1);
cEnd = cData(end);
totalConsumed = cEnd - cStart;

plot(timeAxis, cData, 'LineWidth', 1.5, 'Color', [0.466 0.674 0.188]);
hold on;

% Highlight Start and End points
plot(timeAxis(1), cStart, 'go', 'MarkerFaceColor', 'g', 'MarkerSize', 8);
text(timeAxis(1), cStart, ['  Start: ', num2str(cStart), ' mAh'], 'VerticalAlignment', 'top');
plot(timeAxis(end), cEnd, 'ro', 'MarkerFaceColor', 'r', 'MarkerSize', 8);
text(timeAxis(end), cEnd, ['  End: ', num2str(cEnd), ' mAh'], 'HorizontalAlignment', 'right', 'VerticalAlignment', 'bottom');

% Labels and Measured Time
ylabel('Capacity (mAh)'); 
xlabel(['Time (Total Measured Time: ', char(totalDuration), ')']); 
title(['Total Discharged Capacity (Consumed: ', num2str(totalConsumed), ' mAh)']);
grid on;
ylim([min(cData)-50, max(cData)+100]);

% --- 5. Final Formatting ---
linkaxes([ax1, ax2, ax3, ax4], 'x');
sgtitle('Hexadrone 3.4kg test - Power Analysis');
