import './style';
import { useEffect, useState } from 'preact/hooks';
import { RelayControls } from './RelayControls';
import { DialogPortalRoot } from './components/Dialog';
import { GlobalInfo } from './GlobalInfo';
import { SensorInfo } from './SensorInfo';
import { WifiForm } from './WifiForm';
import { RestartButton } from './RestartButton';

export default function App() {
  const name = getName();

  // set tab name:
  useEffect(() => {
    document.title = `${name}`;
  }, [name]);

  return (
    <>
      <div className="app-root">
        <h1>{name}</h1>
        <hr />
        <RelayControls />
        <hr />
        <GlobalInfo />
        <hr />
        <SensorInfo />
        <hr />
        <WifiForm />
        <hr />
        <RestartButton />
      </div>

      <DialogPortalRoot />
    </>
  );
}
/**
 * Get the domain name of the current page.
 */
const getName = () => {
  // get domain name: eg. http://sunroom2.local -> sunroom2

  const rawDomain = window.location.hostname.split('.')?.[0];

  if (!rawDomain) {
    return 'Unknown';
  }

  // capitalize first letter
  return rawDomain.charAt(0)?.toUpperCase() + rawDomain.slice(1);
};
