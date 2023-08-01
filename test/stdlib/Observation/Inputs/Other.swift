import Observation
import Foundation

@available(macOS 14.0, *)
@MainActor @Observable
public class Model {
    public enum State {
        case initializing
        case running
        case complete
    }

    public struct Updates<Element: Sendable>: AsyncSequence, Sendable {
      let stream: AsyncStream<Element>

      init(stream: AsyncStream<Element>) {
        self.stream = stream
      }

      public struct Iterator: AsyncIteratorProtocol {
        var iterator: AsyncStream<Element>.Iterator

        init(iterator: AsyncStream<Element>.Iterator) {
          self.iterator = iterator
        }

        public mutating func next() async -> Element? {
          return await iterator.next()
        }
      }

      public func makeAsyncIterator() -> Iterator {
        Iterator(iterator: stream.makeAsyncIterator())
      }
    }

    public private(set) var state: State = .initializing

    private func updateStream(state: State) {
    }
}

@available(*, unavailable)
extension Model.Updates.Iterator: Sendable { }
