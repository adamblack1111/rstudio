import { UserDefaultTypes } from 'electron';
import MacPreferences from './mac-preferences';
import WindowsPreferences from './windows-preferences';

export interface PlatformPreferences {
  getValue(
    key: string,
    type?: keyof UserDefaultTypes,
  ): string | number | boolean | unknown[] | Record<string, unknown> | undefined;
  setValue(key: string, type: keyof UserDefaultTypes, value: string): void;
}

const isMacOS = process.platform === 'darwin';

/**
 * MacOS stores periods as · in defaults. When adding a new key, ensure that the key is valid
 * on all platforms.
 */
export const preferenceKeys = {
  fontFixedWidth: isMacOS ? 'font·fixedWidth' : 'General.font.fixedWidth'
}

const getPreferenceManager = () => {
  switch (process.platform) {
    case 'darwin':
      return new MacPreferences();
    case 'win32':
      return new WindowsPreferences();
    default:
      throw Error('unsupported platform');
  }
}

export default getPreferenceManager;