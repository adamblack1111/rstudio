/*
 * panel.tsx
 *
 * Copyright (C) 2019-20 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */



import React from 'react';

import { WidgetProps } from './react';

import './panel.css';

export const Panel: React.FC<WidgetProps> = props => {

  const className = ['pm-horizontal-panel'].concat(props.classes || []).join(' ');

  const children = props.children;

  return (
    <div className={className}>
      {React.Children.map(children, child => <div className='pm-horizontal-panel-cell'>{child}</div>)}
    </div>
  );
};

