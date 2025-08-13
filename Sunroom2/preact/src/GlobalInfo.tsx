import { useEffect, useState } from 'preact/hooks';
import { Section } from './components/Section';

type GlobalInfoResponse = {
  ChipId?: string;
  ResetCounter?: number;
  LastResetReason?: number;
  InternalTemperature?: number;
  CurrentTime?: string;
};

// ESP32 reset reasons mapping
const getResetReasonString = (reason?: number): string => {
  if (reason === undefined) return 'Unknown';

  switch (reason) {
    case 0:
      return 'Reset reason cannot be determined';
    case 1:
      return 'Power-on event';
    case 2:
      return 'Software reset via esp_restart';
    case 3:
      return 'Software reset due to exception/panic';
    case 4:
      return 'Reset due to interrupt watchdog';
    case 5:
      return 'Reset due to task watchdog';
    case 6:
      return 'Reset due to other watchdogs';
    case 7:
      return 'Reset after exiting deep sleep mode';
    case 8:
      return 'Brownout reset';
    case 9:
      return 'Reset over SDIO';
    default:
      return `Unknown reset reason (${reason})`;
  }
};

/**
 * Global info is the current state of the device.
 */
export const GlobalInfo = () => {
  const [globalInfo, setGlobalInfo] = useState<GlobalInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/global-info');
      const json = await data.json();
      setGlobalInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="GlobalInfo" title="Global Info">
      <p>Chip Id: #{globalInfo.ChipId}</p>
      <p>Resets: {globalInfo.ResetCounter}</p>
      <p>
        Last reset reason: {getResetReasonString(globalInfo.LastResetReason)}
      </p>
      <p>Internal temperature: {globalInfo.InternalTemperature}F</p>
      <p>Current time: {globalInfo.CurrentTime}</p>
    </Section>
  );
};
