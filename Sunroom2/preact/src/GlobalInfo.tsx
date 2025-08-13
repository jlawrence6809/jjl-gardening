import { useEffect, useState } from 'preact/hooks';
import { Section } from './components/Section';

type GlobalInfoResponse = {
  ChipId?: string;
  ResetCounter?: number;
  InternalTemperature?: number;
  CurrentTime?: string;
};

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
      <p>Internal temperature: {globalInfo.InternalTemperature}F</p>
      <p>Current time: {globalInfo.CurrentTime}</p>
    </Section>
  );
};
