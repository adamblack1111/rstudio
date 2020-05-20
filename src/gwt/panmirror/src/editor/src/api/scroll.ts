/*
 * rmd.ts
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

import { EditorView } from 'prosemirror-view';
import { findParentNodeOfTypeClosestToPos } from 'prosemirror-utils';

import zenscroll from 'zenscroll';

import { editingRootNodeClosestToPos } from './node';
import { off } from 'codemirror';

export function scrollIntoView(
  view: EditorView,
  pos: number,
  center = true,
  duration?: number,
  offset?: number,
  onDone?: VoidFunction,
) {
  // resolve position and determine container
  const $pos = view.state.doc.resolve(pos);
  const container = editingRootNodeClosestToPos($pos);

  // if we have a container then do the scroll
  if (container) {
    const schema = view.state.schema;
    const containerEl = view.nodeDOM(container.pos) as HTMLElement;
    const parentList = findParentNodeOfTypeClosestToPos($pos, [schema.nodes.ordered_list, schema.nodes.bullet_list]);
    const resultPos = parentList ? parentList.pos : $pos.before();
    const resultNode = view.nodeDOM(resultPos) as HTMLElement;
    if (container && resultNode) {
      const scroller = zenscroll.createScroller(containerEl, duration, offset);
      if (center) {
        scroller.center(resultNode, duration, offset, onDone);
      } else {
        scroller.intoView(resultNode, duration, onDone);
      }
    }
  }
}
