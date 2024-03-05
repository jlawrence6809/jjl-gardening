import './style';
import { useEffect, useState } from 'preact/hooks';

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
  );
}

const GlobalInfo = () => {
  const [globalInfo, setGlobalInfo] = useState({});

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

const SensorInfo = () => {
  const [sensorInfo, setSensorInfo] = useState({});

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

const RelayControls = () => {
  const [relayState, setRelayState] = useState({
    relay_1: 'LOADING',
    relay_2: 'LOADING',
    relay_3: 'LOADING',
    relay_4: 'LOADING',
    relay_5: 'LOADING',
    relay_6: 'LOADING',
    relay_7: 'LOADING',
    relay_8: 'LOADING',
  });

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

  const handleSubmit = async (relay, value) => {
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
        const relay = `relay_${i + 1}`;
        return (
          <ToggleSwitch
            id={relay}
            name={relay}
            label={RELAY_LABELS[relay]}
            value={relayState[relay]}
            onChange={(value) => handleSubmit(relay, value)}
          />
        );
      })}
    </Section>
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

const Section = ({ className = '', title, children }) => {
  return (
    <div className={`Section ${className}`}>
      <h2>{title}</h2>
      {children}
    </div>
  );
};

/**
 * ToggleSwitch
 *
 * @param {String} name
 * @param {String} label
 * @param {"0" | "1" | "LOADING"} value
 * @param {Function} onChange
 * @returns
 */
const ToggleSwitch = ({ name, label, value, onChange }) => {
  const inputId = `hidden-${name}`;

  return (
    <div
      className={`ToggleSwitch ${
        value === 'LOADING' ? 'loading' : value === '1' ? 'on' : 'off'
      }`}
      onClick={() => {
        const input = document.getElementById(inputId);
        const newValue = input.value === '1' ? '0' : '1';
        input.value = newValue;
        onChange(newValue);
      }}
    >
      <input id={inputId} className="hidden-input" name={name} value={value} />
      {label}
    </div>
  );
};
