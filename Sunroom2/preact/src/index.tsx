import './style';
import { useEffect, useState } from 'preact/hooks';
import { createPortal } from 'preact/compat';
import { VNode } from 'preact';
import {
  Rule,
  CurrentSensorActuatorValues,
  InputConditionConfigs,
} from './types';

const PORTAL_ROOT_ID = 'portal-root';

const RELAY_LABELS = {
  relay_1: 'Outlet A',
  relay_2: 'Outlet B',
  relay_3: 'Outlet C',
  relay_4: 'Outlet D',
  relay_5: 'NC',
  relay_6: 'Outdoor Lights',
  relay_7: 'Sunroom Lights',
  relay_8: 'Fan',
};

export default function App() {
  return (
    <>
      <div className="app-root">
        <h1>Sunroom 2</h1>
        <hr />
        <RelayControls />
        <hr />
        <GlobalInfo />
        <hr />
        <SensorInfo />
        <hr />
        <WifiForm />
        <hr />
        <Restart />
      </div>

      <div id={PORTAL_ROOT_ID}></div>
    </>
  );
}

type GlobalInfoResponse = {
  ChipId?: string;
  ResetCounter?: number;
  InternalTemperature?: number;
  CurrentTime?: string;
};

const GlobalInfo = () => {
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

type SensorInfoResponse = {
  Temperature?: number;
  Humidity?: number;
  Light?: number;
  Switch?: number;
};

const SensorInfo = () => {
  const [sensorInfo, setSensorInfo] = useState<SensorInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/sensor-info');
      const json = await data.json();
      setSensorInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="SensorInfo" title="Sensor Info">
      <p>Temperature: {sensorInfo.Temperature}F</p>
      <p>Humidity: {sensorInfo.Humidity}%</p>
      <p>Light: {sensorInfo.Light}</p>
      <p>Switch: {sensorInfo.Switch}</p>
    </Section>
  );
};

type RelayStateValue = '0' | '1' | 'auto' | 'LOADING';
type Relay =
  | 'relay_1'
  | 'relay_2'
  | 'relay_3'
  | 'relay_4'
  | 'relay_5'
  | 'relay_6'
  | 'relay_7'
  | 'relay_8';
type RelayState = Record<Relay, RelayStateValue>;

const NEXT_RELAY_STATE = {
  '0': '1',
  '1': 'auto',
  auto: '0',
} as const;

const RelayControls = () => {
  const [relayState, setRelayState] = useState<RelayState>({
    // relay_1: 'LOADING',
    // relay_2: 'LOADING',
    // relay_3: 'LOADING',
    // relay_4: 'LOADING',
    // relay_5: 'LOADING',
    // relay_6: 'LOADING',
    // relay_7: 'LOADING',
    // relay_8: 'LOADING',
    relay_1: '0',
    relay_2: 'auto',
    relay_3: '0',
    relay_4: '1',
    relay_5: '0',
    relay_6: '1',
    relay_7: '0',
    relay_8: '0',
  });

  const [automateDialogRelay, setAutomateDialogRelay] = useState<Relay | null>(
    null,
  );

  const isLoading = Object.values(relayState).some(
    (value) => value === 'LOADING',
  );

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/relays');
      const json = await data.json();
      setRelayState(json);
    };
    load();
  }, []);

  const handleSubmit = async (relay: Relay, value: RelayStateValue) => {
    const previousState = relayState[relay];
    setRelayState({
      ...relayState,
      [relay]: 'LOADING',
    });

    try {
      const data = new FormData();
      data.append(relay, value);
      const response = await fetch('/relays', {
        method: 'POST',
        body: data,
      });
      const json = await response.json();

      setRelayState(json);
    } catch (_error) {
      setRelayState({
        ...relayState,
        [relay]: previousState,
      });
    }
  };

  return (
    <Section
      className={`RelayForm ${isLoading ? 'loading' : ''}`}
      title="Relay Controls"
    >
      {new Array(8).fill(0).map((_, i) => {
        const relay = `relay_${i + 1}` as Relay;
        const value = relayState[relay];
        const label = RELAY_LABELS[relay];
        const name = relay;

        return (
          // <ToggleSwitch
          //   key={relay}
          //   name={relay}
          //   label={RELAY_LABELS[relay]}
          //   value={relayState[relay]}
          //   onChange={(value) => handleSubmit(relay, value)}
          // />

          <div
            key={relay}
            className={`ToggleSwitch state_${value}`}
            onClick={() => handleSubmit(relay, NEXT_RELAY_STATE[value])}
          >
            <div
              className={'AutomateButton'}
              onClick={(ev) => {
                ev.stopPropagation();
                setAutomateDialogRelay(relay);
              }}
            >
              A
            </div>
            {label}
          </div>
        );
      })}
      <AutomateDialog
        relay={automateDialogRelay}
        onClose={() => setAutomateDialogRelay(null)}
      />
    </Section>
  );
};

type AutomateDialogProps = {
  relay: Relay | null;
  onClose: () => void;
};

const AutomateDialog = ({ relay, onClose }: AutomateDialogProps) => {
  const [rule, setRule] = useState<Rule | null>(null);

  const [currentSensorActuatorValues, setCurrentSensorActuatorValues] =
    useState<CurrentSensorActuatorValues | null>(null);

  const [inputConditionConfigs, setInputConditionConfigs] =
    useState<InputConditionConfigs | null>(null);

  useEffect(() => {
    const load = async () => {
      if (relay === null) return;
      const data = await fetch(`/rules/${relay}`);
      const json = await data.json();
      setRule(json);
    };
    load();
  }, [relay]);

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/current-sensor-actuator-values');
      const json = await data.json();
      setCurrentSensorActuatorValues(json);
    };
    load();
  }, []);

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/input-condition-configs');
      const json = await data.json();
      setInputConditionConfigs(json);
    };
    load();
  }, []);

  if (relay === null) return <></>;

  const label = RELAY_LABELS[relay];

  return (
    <FullScreenDialog onClose={onClose}>
      <div className="AutomateDialog">
        <h3>Automate {label}</h3>
      </div>
    </FullScreenDialog>
  );
};

const WifiForm = () => {
  return (
    <Section title="Wifi Settings">
      <form className="WifiForm" action="/wifi-settings" method="post">
        <div>
          <label for="ssid">Wifi Name</label>
          <input type="text" id="ssid" name="ssid" />
        </div>
        <div>
          <label for="password">Wifi Password</label>
          <input type="password" id="password" name="password" />
        </div>
        <button type="submit">Submit</button>
      </form>
    </Section>
  );
};

const RESTART_SECONDS = 5;

const Restart = () => {
  const [restartSuccess, setRestartSuccess] = useState(false);

  return (
    <div>
      <button
        onClick={async () => {
          try {
            await fetch('/restart', { method: 'POST' });
            setRestartSuccess(true);
            setTimeout(() => {
              setRestartSuccess(false);
            }, RESTART_SECONDS * 1000);
          } catch (error) {
            console.error(error);
            alert('Error restarting');
          }
        }}
      >
        Reset
      </button>
      {restartSuccess && (
        <p>Success, restarting in {RESTART_SECONDS} seconds...</p>
      )}
    </div>
  );
};

type SectionProps = {
  className?: string;
  title: string;
  children: any;
};

const Section = ({ className = '', title, children }: SectionProps) => {
  return (
    <div className={`Section ${className}`}>
      <h2>{title}</h2>
      {children}
    </div>
  );
};

type ToggleSwitchProps = {
  name: string;
  label: string;
  value: RelayStateValue;
  onChange: (value: RelayStateValue) => void;
};

/**
 * ToggleSwitch
 */
const ToggleSwitch = ({ name, label, value, onChange }: ToggleSwitchProps) => {
  const inputId = `hidden-${name}`;

  return (
    <div
      className={`ToggleSwitch state_${value}`}
      onClick={() => {
        const input = document.getElementById(inputId) as HTMLInputElement;
        const newValue = {
          '0': '1',
          '1': 'auto',
          auto: '0',
        }[value];
        input.value = newValue;
        onChange(newValue);
      }}
    >
      <input id={inputId} className="hidden-input" name={name} value={value} />
      {label}
    </div>
  );
};

type FullScreenDialogProps = {
  children: preact.ComponentChild;
  onClose: () => void;
};

const FullScreenDialog = ({ children, onClose }: FullScreenDialogProps) => {
  return (
    <DialogPortal>
      <div className="FullScreenDialog" onClick={onClose}>
        <div className="inner" onClick={(e) => e.stopPropagation()}>
          {children}
        </div>
      </div>
    </DialogPortal>
  );
};

const DialogPortal = ({ children }: { children: VNode<{}> }) => {
  const portalRoot = document.getElementById(PORTAL_ROOT_ID);
  return portalRoot ? createPortal(children, portalRoot) : null;
};
